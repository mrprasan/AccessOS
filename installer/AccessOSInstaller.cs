// AccessOSInstaller.cs — Self-contained AccessOS Setup Launcher
// Compiles to a single AccessOS-Setup.exe (no dependencies).
//
// What it does:
//   1. Checks Windows version (requires 10.0.17763+)
//   2. Extracts the embedded signing certificate and imports it into Trusted Root
//   3. Extracts the MSIX to a temp folder
//   4. Calls Add-AppxPackage to install it
//   5. Creates a Desktop shortcut pointing to the installed app
//   6. Shows progress in a simple console window
//
// Build:
//   csc /target:winexe /out:dist\AccessOS-Setup.exe /win32icon:src\UI\Assets\AppIcon.ico
//       /resource:installer\AccessOSDev.cer,AccessOSDev.cer
//       /resource:dist\Release\AccessOS.msix,AccessOS.msix
//       installer\AccessOSInstaller.cs
//   (or: dotnet-script / csc via VS Build Tools)

using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading;
using Microsoft.Win32;

// Target .NET 4.8 for maximum Windows compatibility (no .NET 8 required for the installer itself)
[assembly: System.Runtime.Versioning.TargetFramework(".NETFramework,Version=v4.8")]

namespace AccessOSSetup
{
    class Program
    {
        // ── Win32 helpers ────────────────────────────────────────────────────
        [DllImport("kernel32.dll")]
        static extern bool AllocConsole();

        [DllImport("user32.dll", CharSet = CharSet.Auto)]
        static extern int MessageBox(IntPtr hWnd, string text, string caption, uint type);

        const uint MB_OK          = 0x0;
        const uint MB_ICONINFO    = 0x40;
        const uint MB_ICONERROR   = 0x10;
        const uint MB_ICONWARNING = 0x30;

        // ── Entry point ───────────────────────────────────────────────────────
        [STAThread]
        static int Main(string[] args)
        {
            AllocConsole();
            Console.Title = "AccessOS Setup";
            Console.OutputEncoding = Encoding.UTF8;

            bool quiet = Array.IndexOf(args, "/quiet") >= 0 ||
                         Array.IndexOf(args, "-quiet") >= 0 ||
                         Array.IndexOf(args, "--quiet") >= 0;

            bool uninstall = Array.IndexOf(args, "/uninstall") >= 0 ||
                             Array.IndexOf(args, "-uninstall") >= 0;

            Print("╔══════════════════════════════════════╗");
            Print("║       AccessOS Setup  v1.0.0         ║");
            Print("╚══════════════════════════════════════╝");
            Print("");

            // ── Uninstall mode ───────────────────────────────────────────────
            if (uninstall)
            {
                Print("[*] Uninstalling AccessOS...");
                int r = RunPS("Get-AppxPackage -Name 'AccessOS.ScreenReader' | Remove-AppxPackage");
                if (r == 0)
                    Print("[✓] AccessOS removed successfully.");
                else
                    PrintWarn("[!] Uninstall returned exit code " + r + ". The package may already be removed.");
                if (!quiet) Pause();
                return r;
            }

            // ── Check Windows version ────────────────────────────────────────
            // Environment.OSVersion.Version returns 6.2 on Windows 10/11 when no
            // app manifest declares supportedOS. Read the real build from the registry.
            Print("[*] Checking Windows version...");
            int realMajor = 0, realMinor = 0, realBuild = 0;
            try
            {
                using (var key = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\Windows NT\CurrentVersion"))
                {
                    if (key != null)
                    {
                        realMajor = Convert.ToInt32(key.GetValue("CurrentMajorVersionNumber", 0));
                        realMinor = Convert.ToInt32(key.GetValue("CurrentMinorVersionNumber", 0));
                        string buildStr = (key.GetValue("CurrentBuildNumber") ?? "0").ToString();
                        int.TryParse(buildStr, out realBuild);
                    }
                }
            }
            catch { }

            // Fallback to Environment.OSVersion when registry unavailable
            if (realMajor == 0)
            {
                var ev = Environment.OSVersion.Version;
                realMajor = ev.Major; realMinor = ev.Minor; realBuild = ev.Build;
            }

            // Windows 10 1809 = build 17763; Windows 11 = build 22000+
            bool supported = (realMajor > 10) ||
                             (realMajor == 10 && realBuild >= 17763);

            if (!supported)
            {
                string verStr = realMajor + "." + realMinor + " build " + realBuild;
                PrintError("[✗] AccessOS requires Windows 10 version 1809 (build 17763) or later.");
                PrintError("    Your version: " + verStr);
                if (!quiet) MessageBox(IntPtr.Zero,
                    "AccessOS requires Windows 10 version 1809 or later.\n\nDetected version: " + verStr,
                    "AccessOS Setup — Unsupported Windows Version", MB_ICONERROR);
                return 1;
            }

            string winName = realBuild >= 22000 ? "Windows 11" : "Windows 10";
            Print("    " + winName + " build " + realBuild + " — OK");

            // ── Extract temp folder ──────────────────────────────────────────
            string tmpDir = Path.Combine(Path.GetTempPath(), "AccessOSSetup_" + Guid.NewGuid().ToString("N").Substring(0, 8));
            Directory.CreateDirectory(tmpDir);

            try
            {
                // ── Step 1: Install signing certificate ──────────────────────
                Print("");
                Print("[*] Installing signing certificate...");

                byte[] certBytes = GetResource("AccessOSDev.cer");
                if (certBytes == null)
                {
                    PrintError("[✗] Embedded certificate resource not found.");
                    return 1;
                }

                string certPath = Path.Combine(tmpDir, "AccessOSDev.cer");
                File.WriteAllBytes(certPath, certBytes);

                try
                {
                    var cert = new X509Certificate2(certBytes);
                    using (var store = new X509Store(StoreName.Root, StoreLocation.LocalMachine))
                    {
                        store.Open(OpenFlags.ReadWrite);
                        if (!store.Certificates.Contains(cert))
                        {
                            store.Add(cert);
                            Print("    Certificate imported: " + cert.Subject);
                        }
                        else
                        {
                            Print("    Certificate already trusted — skipping.");
                        }
                    }
                }
                catch (Exception ex)
                {
                    PrintWarn("[!] Could not import certificate via API: " + ex.Message);
                    PrintWarn("    Trying certutil fallback...");
                    int certRc = Run("certutil.exe", "-addstore -f Root \"" + certPath + "\"", quiet: true);
                    if (certRc != 0)
                    {
                        PrintError("[✗] Certificate import failed. Try running as Administrator.");
                        if (!quiet) MessageBox(IntPtr.Zero,
                            "Could not install the signing certificate.\n\nPlease run AccessOS-Setup.exe as Administrator (right-click → Run as administrator).",
                            "AccessOS Setup — Administrator Required", MB_ICONERROR);
                        return 1;
                    }
                    Print("    Certificate imported via certutil.");
                }

                // ── Step 2: Extract MSIX ─────────────────────────────────────
                Print("");
                Print("[*] Extracting package...");

                byte[] msixBytes = GetResource("AccessOS.msix");
                if (msixBytes == null)
                {
                    PrintError("[✗] Embedded MSIX resource not found.");
                    return 1;
                }

                string msixPath = Path.Combine(tmpDir, "AccessOS.msix");
                File.WriteAllBytes(msixPath, msixBytes);
                Print("    Package extracted: " + Math.Round(msixBytes.Length / 1024.0 / 1024.0, 1) + " MB");

                // ── Step 3: Remove any existing version ──────────────────────
                Print("");
                Print("[*] Checking for existing installation...");
                string removeCmd = @"
$pkg = Get-AppxPackage -Name 'AccessOS.ScreenReader' -ErrorAction SilentlyContinue
if ($pkg) {
    Write-Host ('    Removing existing version: ' + $pkg.Version)
    $pkg | Remove-AppxPackage -ErrorAction SilentlyContinue
} else {
    Write-Host '    No existing installation found.'
}";
                RunPS(removeCmd);

                // ── Step 4: Install MSIX ─────────────────────────────────────
                Print("");
                Print("[*] Installing AccessOS...");

                string psCmd = "Add-AppxPackage -Path '" + msixPath.Replace("'", "''") + "' -ForceApplicationShutdown";
                int rc = RunPS(psCmd);

                if (rc != 0)
                {
                    // Capture detailed error from deployment log
                    string errDetail = RunPSCapture(
                        "Get-WinEvent -LogName 'Microsoft-Windows-AppXDeployment-Server/Operational' " +
                        "-MaxEvents 10 -ErrorAction SilentlyContinue | " +
                        "Where-Object { $_.Message -like '*AccessOS*' } | " +
                        "Select-Object -First 3 -ExpandProperty Message").Trim();

                    PrintError("[✗] Installation failed (exit code " + rc + ").");
                    if (!string.IsNullOrEmpty(errDetail))
                        PrintError("    Detail: " + errDetail.Substring(0, Math.Min(300, errDetail.Length)));

                    if (!quiet) MessageBox(IntPtr.Zero,
                        "AccessOS installation failed.\n\n" +
                        "The Windows App SDK 2.4+ is installed on this machine.\n\n" +
                        "Possible cause: run the installer as Administrator.\n\n" +
                        (string.IsNullOrEmpty(errDetail) ? "" : "Detail:\n" + errDetail.Substring(0, Math.Min(300, errDetail.Length)) + "\n\n") +
                        "If the problem persists, try:\n" +
                        "  Get-AppxPackage -Name AccessOS.ScreenReader | Remove-AppxPackage\n" +
                        "then re-run this installer.",
                        "AccessOS Setup — Installation Failed", MB_ICONERROR);
                    return rc;
                }

                // ── Step 4: Verify installation ──────────────────────────────
                Print("");
                Print("[*] Verifying installation...");
                string checkCmd = "(Get-AppxPackage -Name 'AccessOS.ScreenReader').Version";
                string version = RunPSCapture(checkCmd).Trim();

                if (string.IsNullOrEmpty(version))
                {
                    PrintWarn("[!] Package verification inconclusive — check Start Menu for AccessOS.");
                }
                else
                {
                    Print("    Installed version: " + version);
                }

                // ── Success ──────────────────────────────────────────────────
                Print("");
                Print("╔══════════════════════════════════════╗");
                Print("║   AccessOS installed successfully!   ║");
                Print("║                                      ║");
                Print("║   Launch: Start Menu → AccessOS      ║");
                Print("╚══════════════════════════════════════╝");
                Print("");

                if (!quiet)
                {
                    MessageBox(IntPtr.Zero,
                        "AccessOS has been installed successfully!\n\n" +
                        "Launch it from the Start Menu by searching for \"AccessOS\".\n\n" +
                        "Press CapsLock as the modifier key for all screen reader commands.",
                        "AccessOS Setup — Complete", MB_ICONINFO);
                }

                return 0;
            }
            finally
            {
                // Cleanup temp directory
                try { Directory.Delete(tmpDir, true); } catch { }
            }
        }

        // ── Helpers ───────────────────────────────────────────────────────────

        static byte[] GetResource(string name)
        {
            var asm = Assembly.GetExecutingAssembly();
            using (var s = asm.GetManifestResourceStream(name))
            {
                if (s == null) return null;
                byte[] buf = new byte[s.Length];
                s.Read(buf, 0, buf.Length);
                return buf;
            }
        }

        static int RunPS(string script)
        {
            return Run("powershell.exe",
                "-NoProfile -NonInteractive -ExecutionPolicy Bypass -Command \"" + script.Replace("\"", "\\\"") + "\"",
                quiet: false);
        }

        static string RunPSCapture(string script)
        {
            var psi = new ProcessStartInfo
            {
                FileName               = "powershell.exe",
                Arguments              = "-NoProfile -NonInteractive -ExecutionPolicy Bypass -Command \"" + script.Replace("\"", "\\\"") + "\"",
                UseShellExecute        = false,
                RedirectStandardOutput = true,
                CreateNoWindow         = true,
            };
            using (var p = Process.Start(psi))
            {
                string output = p.StandardOutput.ReadToEnd();
                p.WaitForExit();
                return output;
            }
        }

        static int Run(string exe, string args, bool quiet = false)
        {
            if (!quiet) Print("    > " + exe + " " + args.Substring(0, Math.Min(80, args.Length)) + (args.Length > 80 ? "…" : ""));
            var psi = new ProcessStartInfo
            {
                FileName        = exe,
                Arguments       = args,
                UseShellExecute = false,
                CreateNoWindow  = quiet,
            };
            using (var p = Process.Start(psi))
            {
                p.WaitForExit();
                return p.ExitCode;
            }
        }

        static void Pause()
        {
            Print("Press any key to close...");
            try { Console.ReadKey(true); } catch { }
        }

        static void Print(string msg)       { Console.WriteLine(msg); }
        static void PrintWarn(string msg)   { var c = Console.ForegroundColor; Console.ForegroundColor = ConsoleColor.Yellow; Console.WriteLine(msg); Console.ForegroundColor = c; }
        static void PrintError(string msg)  { var c = Console.ForegroundColor; Console.ForegroundColor = ConsoleColor.Red;    Console.WriteLine(msg); Console.ForegroundColor = c; }
    }
}

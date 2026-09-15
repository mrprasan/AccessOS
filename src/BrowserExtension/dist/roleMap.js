// AccessOS/src/BrowserExtension/src/roleMap.ts
//
// Maps HTML tag names and ARIA roles to AccessOS AccessRole values.
// Used by content.ts when building AccessNodeSnapshot objects.
/** Maps an ARIA role string to an AccessRole. */
const ariaRoleMap = {
    button: 'button',
    checkbox: 'checkbox',
    combobox: 'combobox',
    document: 'document',
    textbox: 'edit',
    searchbox: 'edit',
    spinbutton: 'edit',
    group: 'group',
    heading: 'heading',
    img: 'image',
    link: 'link',
    listbox: 'listbox',
    option: 'listitem',
    listitem: 'listitem',
    menu: 'menu',
    menubar: 'menubar',
    menuitem: 'menuitem',
    menuitemcheckbox: 'menuitem',
    menuitemradio: 'menuitem',
    radio: 'radiobutton',
    tab: 'tab',
    tablist: 'tabcontrol',
    tree: 'tree',
    treeitem: 'treeitem',
    dialog: 'window',
    alertdialog: 'window',
    main: 'document',
    article: 'document',
    region: 'group',
    navigation: 'group',
    banner: 'group',
    contentinfo: 'group',
    complementary: 'group',
    form: 'group',
    search: 'group',
};
/** Maps HTML tag names (lowercase) to an AccessRole when no ARIA role present. */
const tagRoleMap = {
    a: 'link',
    button: 'button',
    input: 'edit', // refined below for type=checkbox/radio
    select: 'combobox',
    textarea: 'edit',
    h1: 'heading',
    h2: 'heading',
    h3: 'heading',
    h4: 'heading',
    h5: 'heading',
    h6: 'heading',
    img: 'image',
    li: 'listitem',
    ol: 'listbox',
    ul: 'listbox',
    menu: 'menu',
    nav: 'group',
    main: 'document',
    article: 'document',
    section: 'group',
    header: 'group',
    footer: 'group',
    form: 'group',
    dialog: 'window',
    table: 'group',
};
/**
 * Derive an AccessRole for the given element.
 * Priority: explicit ARIA role > input type > tag name > 'unknown'.
 */
export function getRoleForElement(el) {
    const ariaRole = el.getAttribute('role')?.trim().toLowerCase();
    if (ariaRole && ariaRole in ariaRoleMap) {
        return ariaRoleMap[ariaRole];
    }
    const tag = el.tagName.toLowerCase();
    // Specialise <input> by type.
    if (tag === 'input') {
        const type = el.type?.toLowerCase();
        if (type === 'checkbox')
            return 'checkbox';
        if (type === 'radio')
            return 'radiobutton';
        if (type === 'button' || type === 'submit' || type === 'reset')
            return 'button';
        return 'edit';
    }
    return tagRoleMap[tag] ?? 'unknown';
}
/**
 * Returns the heading level (1–6) for heading elements, 0 otherwise.
 */
export function getHeadingLevel(el) {
    const tag = el.tagName.toLowerCase();
    const level = { h1: 1, h2: 2, h3: 3, h4: 4, h5: 5, h6: 6 }[tag];
    if (level !== undefined)
        return level;
    const ariaLevel = el.getAttribute('aria-level');
    if (ariaLevel) {
        const n = parseInt(ariaLevel, 10);
        if (n >= 1 && n <= 6)
            return n;
    }
    return 0;
}
//# sourceMappingURL=roleMap.js.map
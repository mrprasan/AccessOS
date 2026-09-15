import type { AccessRole } from './types.js';
/**
 * Derive an AccessRole for the given element.
 * Priority: explicit ARIA role > input type > tag name > 'unknown'.
 */
export declare function getRoleForElement(el: Element): AccessRole;
/**
 * Returns the heading level (1–6) for heading elements, 0 otherwise.
 */
export declare function getHeadingLevel(el: Element): number;
//# sourceMappingURL=roleMap.d.ts.map
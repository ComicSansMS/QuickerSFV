#ifndef INCLUDE_GUARD_QUICKER_SFV_GUI_UI_ENFORCE_HPP
#define INCLUDE_GUARD_QUICKER_SFV_GUI_UI_ENFORCE_HPP

namespace quicker_sfv::gui {

/** Enforces a critical condition.
 * This function as similar to an assert in that it terminates the program if the
 * condition does not hold. Unlike an assert, enforce is always checked and thus
 * suitable for enforcing conditions that could lead to bugs or security issues.
 * @param[in] condition The condition to enforce.
 */
void enforce(bool condition);

}

#endif

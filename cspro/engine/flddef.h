#pragma once

// - FieldBehavior                                  // victor Apr 10, 00
typedef   enum   {
                AsAutoSkip  = 0,     // non-protected, go to the next field when the field is complete (enter is not necesary)
                AsEnter   = 1,       // non-protected, need an enter for go to the next field
                AsProtected = 9      // protected field, the cursor can't stay here
                } FieldBehavior;

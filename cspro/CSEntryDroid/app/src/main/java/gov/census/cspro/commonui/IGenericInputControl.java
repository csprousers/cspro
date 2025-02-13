/***************************************************************************************
 * 
 * CSEntry for Android
 * 
 * Module:		IGenericInputControl.java
 * 
 * Description: Interface contract used to define the behaviors that should
 * 				be expected across all custom CSPro input controls.
 * 
 **************************************************************************************/
package gov.census.cspro.commonui;

import gov.census.cspro.form.CDEField;

public interface IGenericInputControl 
{
	/*
	 * adds a value changed listener to the listener list
	 */
    public void addValueChangedListener(IInputControlValueChangedListener listener);
    /*
     * returns the edit field object this control is working on
     */
    public CDEField getEditField();
    /*
     * initializes the control
     */
    public void init();
    /*
     * notifies all of the listeners associated with this control
     */
    public void notifyChangeListeners(Object value);
   /*
    * removes a value changed listener from the listener list
    */
    public void removeValueChangedListener(IInputControlValueChangedListener listener);
    /*
     * sets the edit field object this control interacts with
     */
    public void setEditField(CDEField editField) throws Exception;
}

package gov.census.cspro.engine;

public class PffStartModeParameter {

    public PffStartModeParameter(int action, double modifyCasePosition)
    {
        this.action = action;
        this.modifyCasePosition = modifyCasePosition;
    }

    public int action;
    public double modifyCasePosition;

    public static final int NO_ACTION = 0;
    public static final int ADD_NEW_CASE = 1;
    public static final int MODIFY_CASE = 2;
    public static final int MODIFY_ERROR = 3;
}

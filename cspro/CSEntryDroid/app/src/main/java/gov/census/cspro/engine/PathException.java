package gov.census.cspro.engine;

public class PathException extends Exception {

    /**
	 * 
	 */
	private static final long serialVersionUID = 6990687125482571831L;

	public PathException(String message) {
        super(message);
    }

    public PathException(String message, Throwable throwable) {
        super(message, throwable);
    }

}
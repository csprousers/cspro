package gov.census.cspro.util;
import com.dropbox.core.DbxRequestConfig;
import com.dropbox.core.http.StandardHttpRequestor;

public class DbxRequestConfigFactory {
    private static DbxRequestConfig sDbxRequestConfig;

    public static DbxRequestConfig getRequestConfig() {
        if (sDbxRequestConfig == null) {
            sDbxRequestConfig = DbxRequestConfig.newBuilder("CSEntry")
                    .withHttpRequestor(new StandardHttpRequestor(StandardHttpRequestor.Config.DEFAULT_INSTANCE))
                    .build();
        }
        return sDbxRequestConfig;
    }
}

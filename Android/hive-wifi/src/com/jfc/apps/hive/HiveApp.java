package com.jfc.apps.hive;

import org.acra.ACRA;
import org.acra.ReportingInteractionMode;
import android.app.Application;
import org.acra.annotation.*;


@ReportsCrashes(
	    formUri = "https://jfcenterprises.cloudant.com/acra-hive/_design/acra-storage/_update/report",
	    reportType = org.acra.sender.HttpSender.Type.JSON,
	    httpMethod = org.acra.sender.HttpSender.Method.PUT,
	    formUriBasicAuthLogin = "pludischerearionlyagodle",
	    formUriBasicAuthPassword = "a657dda10d3147f23e561786db4395c7d559d6f9",
	    formKey = "", // This is required for backward compatibility but not used
	    customReportContent = {
	    		org.acra.ReportField.APP_VERSION_CODE,
	    		org.acra.ReportField.APP_VERSION_NAME,
	    		org.acra.ReportField.ANDROID_VERSION,
	    		org.acra.ReportField.PACKAGE_NAME,
	    		org.acra.ReportField.REPORT_ID,
	    		org.acra.ReportField.BUILD,
	    		org.acra.ReportField.STACK_TRACE
	    },
	    mode = ReportingInteractionMode.TOAST,
	    resToastText = R.string.crash_toast_text
)

public class HiveApp extends Application {
    @Override
    public void onCreate() {
        super.onCreate();
        ACRA.init(this);
    }
}


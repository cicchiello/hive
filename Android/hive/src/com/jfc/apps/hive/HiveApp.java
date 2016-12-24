package com.jfc.apps.hive;

import org.acra.ACRA;
import org.acra.ReportingInteractionMode;
import android.app.Application;
import org.acra.annotation.*;

import com.example.hive.R;

@ReportsCrashes(
	    formUri = "https://jfcenterprises.cloudant.com/acra-vtm/_design/acra-storage/_update/report",
	    reportType = org.acra.sender.HttpSender.Type.JSON,
	    httpMethod = org.acra.sender.HttpSender.Method.POST,
	    formUriBasicAuthLogin = "watincedsonetintirrachat",
	    formUriBasicAuthPassword = "L2OJtWBVyd0HIm43uhBJlhe7",
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


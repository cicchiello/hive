package com.jfc.misc.prop;

import java.util.List;

import org.acra.ACRA;

import com.example.hive.R;
import com.jfc.util.misc.DialogUtils;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;

public class AcraTestProperty implements IPropertyMgr {
	private static final String TAG = AcraTestProperty.class.getName();

	private AlertDialog alert;

	public AcraTestProperty(final Activity activity, ImageButton button) {
    	button.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
		    	Runnable submitBugAction = new Runnable() {
		    		@Override
		    		public void run() {
		    			ACRA.getErrorReporter().handleException(new Exception("This is an ACRA test exception"));
		    		}
		    	};
		    	Runnable cancelAction = new Runnable() {
					@Override
					public void run() {alert=null;}
		    	};
		    	alert = DialogUtils.createAndShowAlertDialog(activity, R.string.acra_test_question, 
		    										    	 R.string.yes, submitBugAction, R.string.cancel, cancelAction);
			}
		});
	}

	public AlertDialog getAlertDialog() {return alert;}
	public boolean onActivityResult(int requestCode, int resultCode, Intent intent) {return false;}
	
	@Override
	public void onPermissionResult(int requestCode, String[] permissions, int[] grantResults) {}
	
}

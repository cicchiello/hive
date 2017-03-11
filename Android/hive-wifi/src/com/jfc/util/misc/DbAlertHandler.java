package com.jfc.util.misc;

import java.util.concurrent.atomic.AtomicBoolean;

import com.jfc.apps.hive.R;

import android.app.Activity;
import android.app.AlertDialog;
import android.os.Handler;
import android.widget.TextView;


public class DbAlertHandler {
	private static final long BLOCK_PERIOD = 5*60*1000;  // block access dialogs for 5 minutes
	private static AtomicBoolean sDbAlertSemaphore = new AtomicBoolean(false);
	private static Handler sDbAlertTimer = null;
	private static AlertDialog sAlert = null;
	private static Activity sOwner = null;

	// must be called from the *thread* that creates any of the valueResid views that will eventually be modified
	public DbAlertHandler() {
		if (sDbAlertTimer == null) 
			sDbAlertTimer = new Handler();
	}

	public void onPause(Activity activity) {
		dismissAlert();
	}

	public void informDbInaccessible(final Activity activity, final String msg, final int valueResid) {
		synchronized (sDbAlertSemaphore) {
			if (!sDbAlertSemaphore.get()) {
				if (sAlert == null) {
					sDbAlertSemaphore.set(true);
	            	activity.runOnUiThread(new Runnable() {
						@Override
						public void run() {
	    	            	setValueAsError(activity, valueResid);
			        		Runnable cancelAction = new Runnable() {
			    				@Override
			    				public void run() {
			    					dismissAlert();
			    					String silenceMsg = activity.getString(R.string.db_inaccessible_ignore_txt);
			    					final Runnable posAction = new Runnable() {
										@Override
										public void run() {
								        	// Stops blocking after a pre-defined period.
					    					dismissAlert();
								            sDbAlertTimer.postDelayed(new Runnable() {public void run() {sDbAlertSemaphore.set(false);}}, BLOCK_PERIOD);
										}
									};
									final Runnable cancelAction = new Runnable() {public void run() {sDbAlertSemaphore.set(false); dismissAlert();}};
			    					sAlert = DialogUtils.createAndShowAlertDialog(activity, 
			    							silenceMsg, android.R.string.ok, posAction, android.R.string.no, cancelAction);
			    				}
			    			};
			    			sAlert = DialogUtils.createAndShowErrorDialog(activity, msg, android.R.string.cancel, cancelAction);
			    			sOwner = activity;
						}
					});
				}
			} else {
            	activity.runOnUiThread(new Runnable() {
					@Override
					public void run() {
    	            	setValueAsError(activity, valueResid);
					}
            	});
			}
		}
	}
	
	private void setValueAsError(Activity activity, int valueResid) {
		if (valueResid != 0) {
			TextView valueTv = (TextView) activity.findViewById(valueResid);
			SplashyText.highlightErrorField(activity, valueTv);
		}
	}
	
	private void dismissAlert() {
		if (sAlert != null) {
			if (!sOwner.isFinishing())
				sAlert.dismiss();
		}
		sAlert = null;
	}
	


}

package com.jfc.misc.prop;

import org.acra.ACRA;

import android.app.Activity;
import android.util.Log;
import android.widget.ImageButton;
import android.widget.TextView;

import com.jfc.srvc.cloud.CouchCmdPush;


public class PropertyBase {
	private static final String TAG = PropertyBase.class.getName();

	protected Activity mActivity;
	protected TextView mTimestampTV;
	protected TextView mValueTV;
	protected String mHiveId;
	
	protected PropertyBase(Activity _activity, String _hiveId, TextView _valueTV, TextView _timestampTV) {
		this.mActivity = _activity;
		this.mValueTV = _valueTV;
		this.mTimestampTV = _timestampTV;
		this.mHiveId = _hiveId;
	}

	
	protected void postToDb(final String sensor, final String instruction) {
		CouchCmdPush.OnCompletion onCompletion = new CouchCmdPush.OnCompletion() {
	    	public void success() {
	    		Log.i(TAG, "Channel Doc PUT success");
	    	}
	    	public void error(String query, String msg) {
	    		Log.e(TAG, "Channel Doc PUT failed: "+msg);
				ACRA.getErrorReporter().handleException(new Exception(query+" failed with msg: "+msg));
	    	}
			public void serviceUnavailable(String msg) {
				// TODO Auto-generated method stub
			}
		};
		
		new CouchCmdPush(mActivity, sensor, instruction, onCompletion).execute();
	}

	// derived class should do whatever is necessary to determine if the Property hasn't been defined or ever touched
	protected boolean isPropertyDefined() {return false;}
	
	// do whatever is necessary to display the undfined state in the ParentView
	protected void displayUndefined() {};
	
	// do whatever is necessary to display the current state in the ParentView
	protected void display() {}
	
	// do whatever is necessary to cleanup and/or save state when the owning activity is paused
	public void onPause() {}
	
}

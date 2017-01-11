package com.jfc.misc.prop;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.TextView;

import com.example.hive.R;
import com.jfc.apps.hive.HiveEnv;
import com.jfc.util.misc.SplashyText;


public class ThreadsPerMeterProperty implements IPropertyMgr {
	private static final String TAG = ThreadsPerMeterProperty.class.getName();

	private static final String THREADS_PER_METER = "THREADS_PER_METER";
	private static final int DEFAULT_THREADS_PER_METER = 1000;
	
    // created on constructions -- no need to save on pause
	private TextView mThreadsPerMeterTv;
	private Activity mActivity;

	// transient variables -- no need to save on pause
	private AlertDialog mAlert;

	
	public static boolean isThreadsPerMeterDefined(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		return SP.contains(THREADS_PER_METER) && 
			!SP.getString(THREADS_PER_METER, Integer.toString(DEFAULT_THREADS_PER_METER)).equals(Integer.toString(DEFAULT_THREADS_PER_METER));
	}
	
	public static int getThreadsPerMeter(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		String valueStr = SP.getString(THREADS_PER_METER, Integer.toString(DEFAULT_THREADS_PER_METER));
		return Integer.parseInt(valueStr);
	}
	
	public static void resetThreadsPerMeter(Activity activity) {
		setThreadsPerMeter(activity, DEFAULT_THREADS_PER_METER);
	}
	
	public ThreadsPerMeterProperty(final Activity activity, final TextView tv, ImageButton button) {
		this.mActivity = activity;
		this.mThreadsPerMeterTv = tv;
		
		if (isThreadsPerMeterDefined(activity)) {
			setThreadsPerMeter(getThreadsPerMeter(activity));
		} else {
			setThreadsPerMeterUndefined();
    	}
		mThreadsPerMeterTv.setBackgroundColor(HiveEnv.ModifiableFieldBackgroundColor);
		
    	button.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				AlertDialog.Builder builder = new AlertDialog.Builder(activity);
				
				builder.setIcon(R.drawable.ic_hive);
				builder.setView(R.layout.threads_dialog);
				builder.setTitle(R.string.threads_per_meter_dialog_title);
				builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
		        	@Override
		        	public void onClick(DialogInterface dialog, int which) {
		        		setThreadsPerMeter(Integer.parseInt(((EditText)mAlert.findViewById(R.id.threadsPerMeterTextValue)).getText().toString()));
		        		SplashyText.highlightModifiedField(mActivity, mThreadsPerMeterTv);
						mAlert.dismiss(); 
						mAlert = null;
		        	}
		        });
		        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
		            @Override
		            public void onClick(DialogInterface dialog, int which) {mAlert.dismiss(); mAlert = null;}
		        });
		        mAlert = builder.show();
		        
		        EditText tv = (EditText) mAlert.findViewById(R.id.threadsPerMeterTextValue);
		        int t = getThreadsPerMeter(mActivity);
		        tv.setText(Integer.toString(t));

			}
		});
	}

	public AlertDialog getAlertDialog() {return mAlert;}

	private void setThreadsPerMeterUndefined() {
		mThreadsPerMeterTv.setText(DEFAULT_THREADS_PER_METER);
	}
	
	private void displayThreadsPerMeter(String msg) {
		mThreadsPerMeterTv.setText(msg);
	}
	
	private void setThreadsPerMeter(int threads) {
		setThreadsPerMeter(mActivity, threads);
		displayThreadsPerMeter(Integer.toString(threads));
	}
	
	public static void setThreadsPerMeter(Activity activity, int threads) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		if (!SP.getString(THREADS_PER_METER, Integer.toString(DEFAULT_THREADS_PER_METER)).equals(Integer.toString(threads))) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(THREADS_PER_METER, Integer.toString(threads));
			editor.commit();
		}
	}

	@Override
	public boolean onActivityResult(int requestCode, int resultCode, Intent intent) {
		return false;
	}

	@Override
	public void onPermissionResult(int requestCode, String[] permissions,
			int[] grantResults) {
		// intentionally unimplemented
	}

}

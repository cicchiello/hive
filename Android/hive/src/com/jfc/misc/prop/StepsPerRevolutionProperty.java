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


public class StepsPerRevolutionProperty implements IPropertyMgr {
	private static final String TAG = StepsPerRevolutionProperty.class.getName();

	private static final String STEPS_PER_REV = "STEPS_PER_REV";
	private static final int DEFAULT_STEPS_PER_REV = 200;
	
    // created on constructions -- no need to save on pause
	private TextView mStepsPerRevTv;
	private Activity mActivity;

	// transient variables -- no need to save on pause
	private AlertDialog mAlert;

	
	public static boolean isStepsPerRevolutionDefined(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		return SP.contains(STEPS_PER_REV) && 
			!SP.getString(STEPS_PER_REV, Integer.toString(DEFAULT_STEPS_PER_REV)).equals(Integer.toString(DEFAULT_STEPS_PER_REV));
	}
	
	public static int getStepsPerRevolution(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		String valueStr = SP.getString(STEPS_PER_REV, Integer.toString(DEFAULT_STEPS_PER_REV));
		return Integer.parseInt(valueStr);
	}
	
	public static void resetStepsPerRevolution(Activity activity) {
		setStepsPerRevolution(activity, DEFAULT_STEPS_PER_REV);
	}
	
	public StepsPerRevolutionProperty(final Activity activity, final TextView tv, ImageButton button) {
		this.mActivity = activity;
		this.mStepsPerRevTv = tv;
		
		if (isStepsPerRevolutionDefined(activity)) {
			setStepsPerRevolution(getStepsPerRevolution(activity));
		} else {
			setStepsPerRevolutionUndefined();
    	}
		mStepsPerRevTv.setBackgroundColor(HiveEnv.ModifiableBackgroundColor);
		
    	button.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				AlertDialog.Builder builder = new AlertDialog.Builder(activity);
				
				builder.setIcon(R.drawable.ic_hive);
				builder.setView(R.layout.steps_per_rev_dialog);
				builder.setTitle(R.string.steps_per_rev_dialog_title);
				builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
		        	@Override
		        	public void onClick(DialogInterface dialog, int which) {
		        		setStepsPerRevolution(Integer.parseInt(((EditText)mAlert.findViewById(R.id.stepsPerRevTextValue)).getText().toString()));
		        		SplashyText.highlightModifiedField(mActivity, mStepsPerRevTv);
						mAlert.dismiss(); 
						mAlert = null;
		        	}
		        });
		        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
		            @Override
		            public void onClick(DialogInterface dialog, int which) {mAlert.dismiss(); mAlert = null;}
		        });
		        mAlert = builder.show();
		        
		        EditText tv = (EditText) mAlert.findViewById(R.id.stepsPerRevTextValue);
		        int t = getStepsPerRevolution(mActivity);
		        tv.setText(Integer.toString(t));
			}
		});
	}

	public AlertDialog getAlertDialog() {return mAlert;}

	private void setStepsPerRevolutionUndefined() {
		mStepsPerRevTv.setText(Integer.toString(DEFAULT_STEPS_PER_REV));
	}
	
	private void displayStepsPerRevolution(String msg) {
		mStepsPerRevTv.setText(msg);
	}
	
	private void setStepsPerRevolution(int steps) {
		setStepsPerRevolution(mActivity, steps);
		displayStepsPerRevolution(Integer.toString(steps));
	}
	
	public static void setStepsPerRevolution(Activity activity, int steps) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		if (!SP.getString(STEPS_PER_REV, Integer.toString(DEFAULT_STEPS_PER_REV)).equals(Integer.toString(steps))) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(STEPS_PER_REV, Integer.toString(steps));
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

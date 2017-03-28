package com.jfc.misc.prop;

import org.acra.ACRA;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.TextView;

import com.jfc.apps.hive.HiveEnv;
import com.jfc.apps.hive.R;
import com.jfc.srvc.cloud.CouchCmdPush;
import com.jfc.util.misc.DialogUtils;
import com.jfc.util.misc.SplashyText;


public class StepsPerSecondProperty extends IntProperty {
	private static final String TAG = StepsPerSecondProperty.class.getName();

	private static final String STEPS_PER_SECOND_PROPNAME = "STEPS_PER_SECOND";
	private static final int DEFAULT_STEPS_PER_SECOND = 200;
	
	private static final String EMBEDDED_STEPS_PER_SECOND_PROPERTY = "steps-per-second";
	
	private String mHiveId, mPropId;
	
	static private String uniqueIdentifier(String base, String hiveId) {
		return base + "|" + hiveId;
	}
	
	public StepsPerSecondProperty(final Activity activity, final TextView tv, ImageButton button) {
		super(activity, uniqueIdentifier(STEPS_PER_SECOND_PROPNAME, HiveEnv.getHiveAddress(activity, ActiveHiveProperty.getActiveHiveName(activity))), DEFAULT_STEPS_PER_SECOND, tv, button);
		this.mHiveId = HiveEnv.getHiveAddress(activity, ActiveHiveProperty.getActiveHiveName(activity));
		this.mPropId = uniqueIdentifier(STEPS_PER_SECOND_PROPNAME, mHiveId);
		
		boolean usingConfigRate = false;
		JSONObject config = UptimeProperty.getEmbeddedConfig(activity, mHiveId);
		if (config.has(STEPS_PER_SECOND_PROPNAME)) {
			try {
				long configTimestamp = Long.parseLong(config.getString(UptimeProperty.EMBEDDED_TIMESTAMP));
				long propertyTimestamp = getTimestamp(activity, uniqueIdentifier(STEPS_PER_SECOND_PROPNAME, mHiveId));
				if (configTimestamp >= propertyTimestamp) {
					int steps_s = Integer.parseInt(config.getString(EMBEDDED_STEPS_PER_SECOND_PROPERTY));
					setStepsPerSecond(steps_s, configTimestamp, mHiveId);
					usingConfigRate = true;
					display(steps_s);
				}
			} catch (NumberFormatException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (JSONException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		
		if (!usingConfigRate) {
			display(getStepsPerSecond(activity, mHiveId));
		}
		
    	button.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				AlertDialog.Builder builder = new AlertDialog.Builder(activity);
				
				builder.setIcon(R.drawable.ic_hive);
				builder.setView(R.layout.one_int_property_dialog);
				builder.setTitle(R.string.steps_per_second_dialog_title);
				builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
		        	@Override
		        	public void onClick(DialogInterface dialog, int which) {
		        		final int stepsPerSecond = Integer.parseInt(((EditText)mAlert.findViewById(R.id.oneIntPropertyTextValue)).getText().toString());
		        		final String stepsPerSecondStr = Integer.toString(stepsPerSecond);
		        		
		        		CouchCmdPush.OnCompletion onCompletion = new CouchCmdPush.OnCompletion() {
							@Override
							public void success() {
								mActivity.runOnUiThread(new Runnable() {
									@Override
									public void run() {
						        		setStepsPerSecond(stepsPerSecond, System.currentTimeMillis()/1000, mHiveId);
						        		SplashyText.highlightModifiedField(mActivity, mTV);
									}
								});
							}
							@Override
							public void error(String query, final String msg) {
								mActivity.runOnUiThread(new Runnable() {
									@Override
									public void run() {
										Runnable cancelAction = new Runnable() {public void run() {mAlert.dismiss(); mAlert = null;}};
										mAlert = DialogUtils.createAndShowErrorDialog(mActivity, msg, android.R.string.cancel, cancelAction);
									}
								});
								ACRA.getErrorReporter().handleException(new Exception(query+" failed with msg: "+msg));
							}
							@Override
							public void serviceUnavailable(final String msg) {
								error("unknown", msg);
							}
						};

						new CouchCmdPush(mActivity, "steps-per-second", stepsPerSecondStr, onCompletion).execute();

						mAlert.dismiss(); 
						mAlert = null;
		        	}
		        });
		        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
		            @Override
		            public void onClick(DialogInterface dialog, int which) {mAlert.dismiss(); mAlert = null;}
		        });
		        mAlert = builder.show();
		        
		        EditText tv = (EditText) mAlert.findViewById(R.id.oneIntPropertyTextValue);
		        int t = getStepsPerSecond(mActivity, mHiveId);
		        tv.setText(Integer.toString(t));
			}
		});
	}


	public static boolean isPrescaleDefined(Context ctxt, String hiveId) {
		return IntProperty.isDefined(ctxt, uniqueIdentifier(STEPS_PER_SECOND_PROPNAME, hiveId), DEFAULT_STEPS_PER_SECOND);
	}
	
	public static int getStepsPerSecond(Context ctxt, String hiveId) {
		return IntProperty.getVal(ctxt, uniqueIdentifier(STEPS_PER_SECOND_PROPNAME, hiveId), DEFAULT_STEPS_PER_SECOND);
	}
	
	public static void resetStepsPerSecond(Context ctxt, String hiveId) {
		IntProperty.reset(ctxt, uniqueIdentifier(STEPS_PER_SECOND_PROPNAME, hiveId), DEFAULT_STEPS_PER_SECOND);
	}
	
	private void setStepsPerSecond(int steps, long timestamp, String hiveId) {
		set(uniqueIdentifier(STEPS_PER_SECOND_PROPNAME, hiveId), steps, timestamp, DEFAULT_STEPS_PER_SECOND);
	}
}

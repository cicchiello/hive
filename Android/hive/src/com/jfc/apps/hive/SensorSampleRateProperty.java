package com.jfc.apps.hive;

import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.TextView;

import com.adobe.xmp.impl.Base64;
import com.example.hive.R;
import com.jfc.misc.prop.ActiveHiveProperty;
import com.jfc.misc.prop.DbCredentialsProperty;
import com.jfc.misc.prop.IPropertyMgr;
import com.jfc.srvc.ble2cld.PostActuatorBackground;
import com.jfc.util.misc.DbAlertHandler;
import com.jfc.util.misc.DialogUtils;
import com.jfc.util.misc.SplashyText;


public class SensorSampleRateProperty implements IPropertyMgr {
	private static final String TAG = SensorSampleRateProperty.class.getName();

	private static final String SAMPLE_RATE = "SAMPLE_RATE";
	private static final int DEFAULT_SAMPLE_RATE = 5;
	
    // created on constructions -- no need to save on pause
	private TextView mSampleRateTv;
	private Activity mActivity;
	private Context mCtxt;
	private DbAlertHandler mDbAlert;

	// transient variables -- no need to save on pause
	private AlertDialog mAlert;

	
	public static boolean isRateDefined(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		return SP.contains(SAMPLE_RATE) && 
			!SP.getString(SAMPLE_RATE, Integer.toString(DEFAULT_SAMPLE_RATE)).equals(Integer.toString(DEFAULT_SAMPLE_RATE));
	}
	
	public static int getRate(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		String valueStr = SP.getString(SAMPLE_RATE, Integer.toString(DEFAULT_SAMPLE_RATE));
		return Integer.parseInt(valueStr);
	}
	
	public static void resetRate(Context ctxt) {
		setRate(ctxt, DEFAULT_SAMPLE_RATE);
	}
	
	public SensorSampleRateProperty(final Activity activity, final TextView tv, ImageButton button, DbAlertHandler _dbAlert) {
		this.mCtxt = activity.getApplicationContext();
		this.mActivity = activity;
		this.mSampleRateTv = tv;
		this.mDbAlert = _dbAlert;
		
		if (isRateDefined(activity)) {
			setRate(getRate(activity));
		} else {
			resetRate();
    	}
		mSampleRateTv.setBackgroundColor(HiveEnv.ModifiableFieldBackgroundColor);
		
    	button.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				AlertDialog.Builder builder = new AlertDialog.Builder(activity);
				
				builder.setIcon(R.drawable.ic_hive);
				builder.setView(R.layout.sample_rate_dialog);
				builder.setTitle(R.string.sample_rate_dialog_title);
				builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
		        	@Override
		        	public void onClick(DialogInterface dialog, int which) {
		        		final int rateValue = Integer.parseInt(((EditText)mAlert.findViewById(R.id.sampleRateTextValue)).getText().toString());
		        		final String rateValueStr = Integer.toString(rateValue);
		        		
		        		OnCompletion onCompletion = new OnCompletion() {
							@Override
							public void success() {
								mActivity.runOnUiThread(new Runnable() {
									@Override
									public void run() {
						        		setRate(rateValue);
						        		SplashyText.highlightModifiedField(mActivity, mSampleRateTv);
									}
								});
							}
							@Override
							public void error(final String msg) {
								mActivity.runOnUiThread(new Runnable() {
									@Override
									public void run() {
										Runnable cancelAction = new Runnable() {public void run() {mAlert.dismiss(); mAlert = null;}};
										mAlert = DialogUtils.createAndShowErrorDialog(mCtxt, msg, android.R.string.cancel, cancelAction);
									}
								});
							}
							@Override
							public void serviceUnavailable(final String msg) {
								mDbAlert.informDbInaccessible(mActivity, msg, mSampleRateTv.getId());
							}
						};

						postToDb(rateValueStr, onCompletion);
						mAlert.dismiss(); 
						mAlert = null;
		        	}
		        });
		        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
		            @Override
		            public void onClick(DialogInterface dialog, int which) {mAlert.dismiss(); mAlert = null;}
		        });
		        mAlert = builder.show();
		        
		        EditText tv = (EditText) mAlert.findViewById(R.id.sampleRateTextValue);
		        int t = getRate(mActivity);
		        tv.setText(Integer.toString(t));
			}
		});
	}

	public AlertDialog getAlertDialog() {return mAlert;}

	private void resetRate() {
		mSampleRateTv.setText(Integer.toString(DEFAULT_SAMPLE_RATE));
	}
	
	private void displayRate(String msg) {
		mSampleRateTv.setText(msg);
	}
	
	private void setRate(int rate) {
		setRate(mActivity, rate);
		displayRate(Integer.toString(rate));
	}
	
	public static void setRate(Context ctxt, int steps) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		if (!SP.getString(SAMPLE_RATE, Integer.toString(DEFAULT_SAMPLE_RATE)).equals(Integer.toString(steps))) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(SAMPLE_RATE, Integer.toString(steps));
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
	
	interface OnCompletion {
		public void success();
		public void error(String msg);
		public void serviceUnavailable(String msg);
	}
	
	private void postToDb(final String valueStr, final OnCompletion onCompletion) {
		String ActiveHive = ActiveHiveProperty.getActiveHiveProperty(mCtxt);
		String HiveId = HiveEnv.getHiveAddress(mCtxt, ActiveHive);
		boolean haveHiveId = HiveId != null;
		
		if (haveHiveId) {
			HiveId = HiveId.replace(':', '-');
			
			PostActuatorBackground.ResultCallback onPostCompletion = new PostActuatorBackground.ResultCallback() {
				@Override
		    	public void success(String id, String rev) {
					onCompletion.success();
				}
				
				@Override
				public void error(String msg) {
					onCompletion.error(msg);
				}

				@Override
				public void serviceUnavailable(String msg) {
					onCompletion.serviceUnavailable(msg);
				}
			};

			JSONObject doc = new JSONObject();
			try {
				doc.put("hiveid", HiveId);
				doc.put("sensor", "sample-rate");
				doc.put("timestamp", Long.toString(System.currentTimeMillis()));
				doc.put("value", valueStr);
			} catch (JSONException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			
			String cloudantUser = DbCredentialsProperty.getCloudantUser(mCtxt);
			String dbName = DbCredentialsProperty.getDbName(mCtxt);
			String dbUrl = DbCredentialsProperty.CouchDbUrl(cloudantUser, dbName);
			
			String dbKey = DbCredentialsProperty.getDbKey(mCtxt);
			String dbPswd = DbCredentialsProperty.getDbPswd(mCtxt);
			String authToken = Base64.encode(dbKey+":"+dbPswd);
			
			new PostActuatorBackground(dbUrl, authToken, doc, onPostCompletion).execute();
		} else {
			onCompletion.equals("HiveId isn't known yet; please pair with a hive first");
		}
	}

}

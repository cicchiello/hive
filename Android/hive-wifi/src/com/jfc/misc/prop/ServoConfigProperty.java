package com.jfc.misc.prop;

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
import android.widget.ImageView;
import android.widget.TextView;

import com.jfc.apps.hive.HiveEnv;
import com.jfc.apps.hive.R;
import com.jfc.misc.prop.ActiveHiveProperty;
import com.jfc.misc.prop.IPropertyMgr;
import com.jfc.srvc.cloud.CouchCmdPush;
import com.jfc.util.misc.DbAlertHandler;
import com.jfc.util.misc.DialogUtils;
import com.jfc.util.misc.SplashyText;


public class ServoConfigProperty implements IPropertyMgr {
	private static final String TAG = ServoConfigProperty.class.getName();

	private static final String TRIP_TEMP_C = "TRIP_TEMP";
	private static final String CLOCKWISE_ON_TEMP_GT = "CLOCKWISE";
	private static final String LOWER_TICK_LIMIT = "LOWER_TICK_LIMIT";
	private static final String UPPER_TICK_LIMIT = "UPPER_TICK_LIMIT";
	private static final double DEFAULT_TRIP_TEMP_C = 37.7;
	private static final String DEFAULT_CLOCKWISE_ON_TEMP_GT = "1";
	private static final int DEFAULT_LOWER_TICK_LIMIT = 40;
	private static final int DEFAULT_UPPER_TICK_LIMIT = 43;
	
    // created on constructions -- no need to save on pause
	private TextView mTripTempTv;
	private Activity mActivity;
	private Context mCtxt;
	private String mHiveId;
	private DbAlertHandler mDbAlert;
	private boolean mAlertDirIsClockwise;

	// transient variables -- no need to save on pause
	private AlertDialog mAlert;

	
	public static boolean isDefined(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		return SP.contains(TRIP_TEMP_C) && 
			!SP.getString(TRIP_TEMP_C, Double.toString(DEFAULT_TRIP_TEMP_C)).equals(Double.toString(DEFAULT_TRIP_TEMP_C));
	}
	
	public static double getTripTemp(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		String valueStr = SP.getString(TRIP_TEMP_C, Double.toString(DEFAULT_TRIP_TEMP_C));
		return Double.parseDouble(valueStr);
	}
	
	public static boolean getIsClockwise(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		String valueStr = SP.getString(CLOCKWISE_ON_TEMP_GT, DEFAULT_CLOCKWISE_ON_TEMP_GT);
		return valueStr.equals("1") ? true : false;
	}
	
	public static int getLowerTick(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		String valueStr = SP.getString(LOWER_TICK_LIMIT, Integer.toString(DEFAULT_LOWER_TICK_LIMIT));
		return Integer.parseInt(valueStr);
	}
	
	public static int getUpperTick(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		String valueStr = SP.getString(UPPER_TICK_LIMIT, Integer.toString(DEFAULT_UPPER_TICK_LIMIT));
		return Integer.parseInt(valueStr);
	}
	
	public void resetConfig(Context ctxt) {
		displayTripTemp(DEFAULT_TRIP_TEMP_C);
	}
	
	public static String getHiveUpdateInstruction(Context ctxt, String hiveId) {
		String tempStr = Integer.toString((int) (getTripTemp(ctxt)+0.5));
		String clockwiseStr = getIsClockwise(ctxt) ? "CW" : "CCW";
		String minTicks = Integer.toString(getLowerTick(ctxt));
		String maxTicks = Integer.toString(getUpperTick(ctxt));
		String msg = "temp|dir|minTicks|maxTicks|"+tempStr+"|"+clockwiseStr+"|"+minTicks+"|"+maxTicks;
		return msg;
	}

	public ServoConfigProperty(final Activity activity, final TextView tv, ImageButton button, DbAlertHandler _dbAlert) {
		this.mCtxt = activity.getApplicationContext();
		this.mActivity = activity;
		this.mTripTempTv = tv;
		this.mDbAlert = _dbAlert;
		this.mHiveId = HiveEnv.getHiveAddress(activity, ActiveHiveProperty.getActiveHiveName(activity));
		
		if (isDefined(activity)) {
			setTripTemp(getTripTemp(activity));
		} else {
			resetConfig(activity);
    	}
		mTripTempTv.setBackgroundColor(HiveEnv.ModifiableFieldBackgroundColor);
		
    	button.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				AlertDialog.Builder builder = new AlertDialog.Builder(activity);
				
				builder.setIcon(R.drawable.ic_hive);
				builder.setView(R.layout.servo_config_dialog);
				builder.setTitle(R.string.servo_config_dialog_title);
				builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
		        	@Override
		        	public void onClick(DialogInterface dialog, int which) {
		        		double tempC = Double.parseDouble(((EditText)mAlert.findViewById(R.id.trip_temp_text)).getText().toString());
		        		boolean clockwise = mAlertDirIsClockwise;
		        		int tickLower = Integer.parseInt(((EditText)mAlert.findViewById(R.id.tick_lower_text)).getText().toString());
		        		int tickUpper = Integer.parseInt(((EditText)mAlert.findViewById(R.id.tick_upper_text)).getText().toString());
		        		
		        		setTripTemp(tempC);
		        		setIsClockwise(clockwise);
		        		setTickLower(tickLower);
		        		setTickUpper(tickUpper);
		        		
		        		SplashyText.highlightModifiedField(mActivity, mTripTempTv);
		        		
		        		CouchCmdPush.OnCompletion onCompletion = new CouchCmdPush.OnCompletion() {
							@Override
							public void success() {
								mActivity.runOnUiThread(new Runnable() {
									@Override
									public void run() {
										Log.i(TAG, "success");
									}
								});
							}
							@Override
							public void error(final String msg) {
								mActivity.runOnUiThread(new Runnable() {
									@Override
									public void run() {
										Runnable cancelAction = new Runnable() {public void run() {mAlert.dismiss(); mAlert = null;}};
										mAlert = DialogUtils.createAndShowErrorDialog(mActivity, msg, android.R.string.cancel, cancelAction);
									}
								});
							}
							@Override
							public void serviceUnavailable(final String msg) {
								mDbAlert.informDbInaccessible(mActivity, msg, mTripTempTv.getId());
							}
						};

						String instruction = getHiveUpdateInstruction(mCtxt, mHiveId);
						String sensor = "latch-config";
						new CouchCmdPush(mCtxt, sensor, instruction, onCompletion).execute();
						
						mAlert.dismiss(); 
						mAlert = null;
		        	}
		        });
		        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
		            @Override
		            public void onClick(DialogInterface dialog, int which) {mAlert.dismiss(); mAlert = null;}
		        });
		        mAlert = builder.show();
		        
		        EditText tempTv = (EditText) mAlert.findViewById(R.id.trip_temp_text);
		        final ImageView clockwiseButton = (ImageView) mAlert.findViewById(R.id.dir_button);
		        clockwiseButton.setOnClickListener(new View.OnClickListener() {
					@Override
					public void onClick(View v) {
						int drawable = mAlertDirIsClockwise ? R.drawable.toggle_acw : R.drawable.toggle_cw;
						clockwiseButton.setImageDrawable(mActivity.getDrawable(drawable));
						mAlertDirIsClockwise = !mAlertDirIsClockwise;
					}
				});
		        EditText lowerTickTv = (EditText) mAlert.findViewById(R.id.tick_lower_text);
		        EditText upperTickTv = (EditText) mAlert.findViewById(R.id.tick_upper_text);
		        
		        tempTv.setText(Integer.toString((int) (getTripTemp(mCtxt)+0.5)));
				int drawable = getIsClockwise(mCtxt)? R.drawable.toggle_cw : R.drawable.toggle_acw;
				clockwiseButton.setImageDrawable(mActivity.getDrawable(drawable));
				lowerTickTv.setText(Integer.toString(getLowerTick(mCtxt)));
				upperTickTv.setText(Integer.toString(getUpperTick(mCtxt)));
			}
		});
	}

	public AlertDialog getAlertDialog() {return mAlert;}

	private void displayTripTemp(double tempC) {
		String celciusStr = mCtxt.getString(R.string.celcius);
		mTripTempTv.setText("trip @ "+tempC+" "+celciusStr);
	}
	
	private void setTripTemp(double tempC) {
		setTripTemp(mCtxt, tempC);
		displayTripTemp(tempC);
	}
	
	private void setIsClockwise(boolean clockwise) {
		setIsClockwise(mCtxt, clockwise);
	}
	
	private void setTickLower(int ticks) {
		setTickLower(mCtxt, ticks);
	}

	private void setTickUpper(int ticks) {
		setTickUpper(mCtxt, ticks);
	}
	
	public static void setTripTemp(Context ctxt, double tempC) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		if (!SP.getString(TRIP_TEMP_C, Double.toString(DEFAULT_TRIP_TEMP_C)).equals(Double.toString(tempC))) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(TRIP_TEMP_C, Double.toString(tempC));
			editor.commit();
		}
	}

	public static void setIsClockwise(Context ctxt, boolean clockwise) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		if (!SP.getString(CLOCKWISE_ON_TEMP_GT, DEFAULT_CLOCKWISE_ON_TEMP_GT).equals(clockwise?"1":"0")) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(CLOCKWISE_ON_TEMP_GT, clockwise?"1":"0");
			editor.commit();
		}
	}

	public static void setTickLower(Context ctxt, int ticks) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		if (!SP.getString(LOWER_TICK_LIMIT, Integer.toString(DEFAULT_LOWER_TICK_LIMIT)).equals(Integer.toString(ticks))) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(LOWER_TICK_LIMIT, Integer.toString(ticks));
			editor.commit();
		}
	}

	public static void setTickUpper(Context ctxt, int ticks) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		if (!SP.getString(UPPER_TICK_LIMIT, Integer.toString(DEFAULT_UPPER_TICK_LIMIT)).equals(Integer.toString(ticks))) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(UPPER_TICK_LIMIT, Integer.toString(ticks));
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
	
}

package com.jfc.apps.hive;

import java.util.concurrent.atomic.AtomicBoolean;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.TextView;

import com.jfc.misc.prop.DbCredentialsProperty;
import com.jfc.misc.prop.ExclusiveButtonWrapper;
import com.jfc.misc.prop.PropertyBase;
import com.jfc.misc.prop.StepsPerRevolutionProperty;
import com.jfc.misc.prop.StepsPerSecondProperty;
import com.jfc.misc.prop.ThreadsPerMeterProperty;
import com.jfc.srvc.cloud.PollSensorBackground;
import com.jfc.srvc.cloud.PollSensorBackground.OnSaveValue;
import com.jfc.util.misc.DbAlertHandler;
import com.jfc.util.misc.SplashyText;

public class MotorProperty extends PropertyBase {
	private static final String TAG = MotorProperty.class.getName();

	private static final String MOTOR_ACTION_PROPERTY = "MOTOR_ACTION_PROPERTY";
	private static final String MOTOR_TIMESTAMP_PROPERTY = "MOTOR_TIMESTAMP_PROPERTY";
	private static final String DEFAULT_MOTOR_TIMESTAMP = "0";
	
	protected static final String MOTOR_ACTION_STOPPED = "stopped";
	protected static final String MOTOR_ACTION_MOVING = "moving";
	protected static final String MOTOR_ACTION_PENDING_MOVE = "pending";
	protected static final String DEFAULT_MOTOR_ACTION = MOTOR_ACTION_STOPPED;
	
	protected ExclusiveButtonWrapper mButton;
	protected AlertDialog mAlert;
	protected int mMotorIndex;
	protected DbAlertHandler mDbAlertHandler;
	
	public static double stepsToLinearDistance(Activity activity, long steps) {
		double lsteps = steps;
		double threadsPerMeter = ThreadsPerMeterProperty.getThreadsPerMeter(activity);
		double stepsPerThread = StepsPerRevolutionProperty.getStepsPerRevolution(activity);
		return lsteps/stepsPerThread/threadsPerMeter;
	}

	public static long linearDistanceToSteps(Activity activity, double distanceMeters) {
		double threadsPerMeter = ThreadsPerMeterProperty.getThreadsPerMeter(activity);
		double stepsPerThread = StepsPerRevolutionProperty.getStepsPerRevolution(activity);
		double steps = distanceMeters*threadsPerMeter*stepsPerThread;
		return steps >= 0 ? (long) (steps+0.5) : (long) (steps -0.5);
	}
	
	public MotorProperty(Activity _activity, String _hiveId, int _motorIndex, TextView _valueTV, ImageButton button, TextView _timestampTV) {
		super(_activity, _hiveId, _valueTV, _timestampTV);
		this.mButton = new ExclusiveButtonWrapper(button, "motor"+Integer.toString(_motorIndex));
		this.mMotorIndex = _motorIndex;

		// make sure the motor is known to be stopped
		MotorProperty.setMotorProperty(mActivity, mHiveId, mMotorIndex, MOTOR_ACTION_STOPPED, 
				System.currentTimeMillis()/1000);

		if (!isPropertyDefined())
			displayUndefined();
		else 
			display();

		// must be called from the *thread* that creates any of the valueResid views that will eventually be modified
		mDbAlertHandler = new DbAlertHandler();

    	button.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {showDialog();}
		});
	}

	
	protected void showDialog() {
		if (mAlert == null) {
			AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
			
			builder.setIcon(R.drawable.ic_hive);
			builder.setView(R.layout.motor_dialog);
			builder.setTitle("Motor "+Integer.toString(mMotorIndex+1)+" distance (mm)");
			builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
	        	@Override
	        	public void onClick(DialogInterface dialog, int which) {
					EditText et = (EditText) mAlert.findViewById(R.id.textValue);
					double linearDistanceMillimeters = Long.parseLong(et.getText().toString());
					long steps = linearDistanceToSteps(mActivity, linearDistanceMillimeters/1000.0);
	
					if (steps != 0) {
						setMotorProperty(mActivity, mHiveId, mMotorIndex, MOTOR_ACTION_PENDING_MOVE, 
								         Long.toString(System.currentTimeMillis()/1000));
						
						String sensor = "motor"+Integer.toString(mMotorIndex)+"-target";
						postToDb(sensor, Long.toString(steps));
						display();
						startPolling((int) (Math.abs(steps)/StepsPerSecondProperty.getStepsPerSecond(mActivity, mHiveId)));
					}
					
					mAlert.dismiss(); 
					mAlert = null;
	        	}
	        });
	        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
	            @Override
	            public void onClick(DialogInterface dialog, int which) {mAlert.dismiss(); mAlert = null;}
	        });
	        mAlert = builder.show();
	        
	        EditText tv = (EditText) mAlert.findViewById(R.id.textValue);
	        tv.setText("0");
	
	        mAlert.findViewById(R.id.plus).setOnClickListener(new View.OnClickListener() {public void onClick(View v) {inc();}});
	        mAlert.findViewById(R.id.plusPlus).setOnClickListener(new View.OnClickListener() {public void onClick(View v) {inc100();}});
	        mAlert.findViewById(R.id.minus).setOnClickListener(new View.OnClickListener() {public void onClick(View v) {dec();}});
	        mAlert.findViewById(R.id.minusMinus).setOnClickListener(new View.OnClickListener() {public void onClick(View v) {dec100();}});
		}
	}

	
	public void pollCloud() {
		String dbUrl = DbCredentialsProperty.getCouchLogDbUrl(mActivity);
		PollSensorBackground.ResultCallback onCompletion = 
			getMotorOnCompletion(mValueTV.getId(), mTimestampTV.getId(), new OnSaveValue() {
				@Override
				public void save(Activity activity, String objId, String value, long timestamp_s) {
					if (value.startsWith(MOTOR_ACTION_MOVING)) {
						// determine if the value should be ignored
						long steps = Long.parseLong(value.substring(MOTOR_ACTION_MOVING.length()));
						long shouldHaveTaken_s = Math.abs(steps)/StepsPerSecondProperty.getStepsPerSecond(mActivity, mHiveId);
						long shouldHaveFinishedBy_s = timestamp_s + 2*shouldHaveTaken_s; // give it twice as long as it should have
						long now_s = System.currentTimeMillis()/1000;
						if (now_s > shouldHaveFinishedBy_s) { 
							// ignore it!
							value = MOTOR_ACTION_STOPPED;
						}
					} else {
						String prevValue = getMotorAction(mActivity, mHiveId, mMotorIndex);
						if (prevValue.equals(MOTOR_ACTION_PENDING_MOVE)) {
							// determine if the value should be ignored
							long pendingTimestamp_s = getMotorTimestamp(mActivity, mHiveId, mMotorIndex);
							long shouldHaveTaken_s = 10; // pending case shouldn't stay pending for more than a few seconds
							long shouldHaveFinishedBy_s = pendingTimestamp_s + 2*shouldHaveTaken_s; // give it twice as long as it should have
							long now_s = System.currentTimeMillis()/1000;
							if (now_s > shouldHaveFinishedBy_s) { 
								// ignore the pending state -- it's been too long; stick with whatever the cloud call returned
							} else {
								// still within pending window, so override the result from the cloud call
								value = prevValue;
								timestamp_s = pendingTimestamp_s;
							}
						}
					}
					MotorProperty.setMotorProperty(mActivity, mHiveId, mMotorIndex, value, timestamp_s);
                    display();
				}
			});
		new PollSensorBackground(dbUrl, PollSensorBackground.createQuery(mHiveId, "motor"+Integer.toString(mMotorIndex)), 
				onCompletion).execute();
	}

	
	protected void inc() {
		EditText tv = (EditText) mAlert.findViewById(R.id.textValue);
		String valueStr = tv.getText().toString();
		int n = Integer.parseInt(valueStr)+1;
		tv.setText(Integer.toString(n));
		SplashyText.highlightModifiedField(mActivity, tv);
	}
	
	protected void inc100() {
		EditText tv = (EditText) mAlert.findViewById(R.id.textValue);
		String valueStr = tv.getText().toString();
		int n = Integer.parseInt(valueStr)+100;
		tv.setText(Integer.toString(n));
		SplashyText.highlightModifiedField(mActivity, tv);
	}
	
	protected void dec() {
		EditText tv = (EditText) mAlert.findViewById(R.id.textValue);
		String valueStr = tv.getText().toString();
		int n = Integer.parseInt(valueStr)-1;
		tv.setText(Integer.toString(n));
		SplashyText.highlightModifiedField(mActivity, tv);
	}
	
	protected void dec100() {
		EditText tv = (EditText) mAlert.findViewById(R.id.textValue);
		String valueStr = tv.getText().toString();
		int n = Integer.parseInt(valueStr)-100;
		tv.setText(Integer.toString(n));
		SplashyText.highlightModifiedField(mActivity, tv);
	}

	
	private AtomicBoolean mStopPoller = new AtomicBoolean(false);
	private Runnable mPoller = null;
	private Thread mPollerThread = null;
	protected void startPolling(final int runningDuration_s) {
		if (mPoller == null) {
			mPoller = new Runnable() {
				@Override
				public void run() {
					// check about once a second whenever this activity is running
					
					// first, wait for evidence that the motor is moving...
					int messageDeliveryTime_s = 10;
					int fudge_s = 20; // big fudge factor to account for vagaries of message delivery
					int timeout_s = messageDeliveryTime_s + fudge_s;
					boolean done = false;
					mStopPoller.set(false);
					
					// first have to wait until I see evidence of the motor moving -- then wait until it stops
					while (!mStopPoller.get() && !done) {
						try {
							Thread.sleep(1000);
						} catch (InterruptedException e) {
							// TODO Auto-generated catch block
							e.printStackTrace();
						}
	
						pollCloud();

						String action = getMotorAction(mActivity, mHiveId, mMotorIndex);
						done = (timeout_s-- <= 0) || action.startsWith(MOTOR_ACTION_MOVING);
					}

					mActivity.runOnUiThread(new Runnable() {public void run() {display();}});
					
					if (!mStopPoller.get()) {
						// next wait for evidence that the motor has stopped
						timeout_s = runningDuration_s + messageDeliveryTime_s + fudge_s;
						done = false;
						
						// first have to wait until I see evidence of the motor moving -- then wait until it stops
						while (!mStopPoller.get() && !done) {
							try {
								Thread.sleep(1000);
							} catch (InterruptedException e) {
								// TODO Auto-generated catch block
								e.printStackTrace();
							}
		
							pollCloud();

							String action = getMotorAction(mActivity, mHiveId, mMotorIndex);
							done = (timeout_s-- <= 0) || !action.startsWith(MOTOR_ACTION_MOVING);
						}

						mActivity.runOnUiThread(new Runnable() {public void run() {display();}});
					}
					Log.i(TAG, "Done");
					cancelPoller();
				}
			};
			mStopPoller.set(false);
			mPollerThread = new Thread(mPoller, "MotorCompletionPoller");
			mPollerThread.start();
		}
	}
	
	private void cancelPoller() {
		mStopPoller.set(true);
		mPollerThread = null;
		mPoller = null;
	}
	
	public void onPause() {
		cancelPoller();
		if (mAlert != null)
			mAlert.dismiss();
		mAlert = null;
	}


	// derived class should do whatever is necessary to determine if the Property hasn't been defined or ever touched
	protected boolean isPropertyDefined() {
		return isMotorPropertyDefined(mActivity, mHiveId, mMotorIndex);
	}
	
	// do whatever is necessary to display the undefined state in the ParentView
	protected void displayUndefined() {
		Log.i(TAG, "Set display undefined");
		long timestampMillis = 0;
		String action = MOTOR_ACTION_STOPPED;
		HiveEnv.setValueWithSplash(mActivity, mValueTV.getId(), mTimestampTV.getId(), action, false, timestampMillis);
	};
	
	// do whatever is necessary to display the current state in the ParentView
	protected void display() {
		long timestamp_s = getMotorTimestamp(mActivity, mHiveId, mMotorIndex);

		String action = getMotorAction(mActivity, mHiveId, mMotorIndex);
		boolean isStopped = action.equals(MOTOR_ACTION_STOPPED);
		if (!isStopped) {
			long shouldHaveTaken_s = 10; // pending case shouldn't stay pending for more than a few seconds
			if (action.startsWith(MOTOR_ACTION_MOVING)) {
				// determine if the property value should be ignored
				long steps = Long.parseLong(action.substring(MOTOR_ACTION_MOVING.length()));
				shouldHaveTaken_s = Math.abs(steps)/StepsPerSecondProperty.getStepsPerSecond(mActivity, mHiveId);
				action = MOTOR_ACTION_MOVING;
			}
			long shouldHaveFinishedBy_s = timestamp_s + 2*shouldHaveTaken_s; // give it twice as long as it should have
			long now_s = System.currentTimeMillis()/1000;
			if (now_s > shouldHaveFinishedBy_s) { 
				// ignore it!
				isStopped = true;
			}
		}
		
		if (isStopped) {
			mButton.enableButton();
		} else {
			mButton.disableButton();
		}
		HiveEnv.setValueWithSplash(mActivity, mValueTV.getId(), mTimestampTV.getId(), action, true, timestamp_s);
	}

	private PollSensorBackground.ResultCallback getMotorOnCompletion(final int valueResid, final int timestampResid, 
			final OnSaveValue saver) {
		PollSensorBackground.ResultCallback onCompletion = new PollSensorBackground.ResultCallback() {
			@Override
	    	public void report(final String objId, String sensorType, final String timestampStr, final String actionStr) {
				mActivity.runOnUiThread(new Runnable() {
					@Override
					public void run() {
                        saver.save(mActivity, objId, actionStr, Long.parseLong(timestampStr));
//                        display();
					}
				});
			}
			
			@Override
			public void error(final String msg) {
				String errMsg = "Attempt to get Motor location failed with this message: "+msg;
				mDbAlertHandler.informDbInaccessible(mActivity, errMsg, valueResid);
			}

			@Override
			public void dbAccessibleError(final String msg) {
				mDbAlertHandler.informDbInaccessible(mActivity, msg, valueResid);
			}
		};
		return onCompletion;
	}

	private static String uniqueIdentifier(String base, String hiveId, int motorIndex) {
		return base+"|"+hiveId+"|"+motorIndex;
	}

	
	public static boolean isMotorPropertyDefined(Activity activity, String hiveId, int motorIndex) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		return  (SP.contains(uniqueIdentifier(MOTOR_ACTION_PROPERTY, hiveId, motorIndex)) && 
				 !SP.getString(uniqueIdentifier(MOTOR_ACTION_PROPERTY, hiveId, motorIndex), DEFAULT_MOTOR_ACTION).equals(DEFAULT_MOTOR_ACTION)) ||
				(SP.contains(uniqueIdentifier(MOTOR_TIMESTAMP_PROPERTY, hiveId, motorIndex)) &&
				 !SP.getString(uniqueIdentifier(MOTOR_TIMESTAMP_PROPERTY, hiveId, motorIndex), DEFAULT_MOTOR_TIMESTAMP).equals(DEFAULT_MOTOR_TIMESTAMP));
	}
	
	
	public static long getMotorTimestamp(Activity activity, String hiveId, int motorIndex) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		String v = SP.getString(uniqueIdentifier(MOTOR_TIMESTAMP_PROPERTY, hiveId, motorIndex), DEFAULT_MOTOR_TIMESTAMP);
		try {
			return Long.parseLong(v);
		} catch (NumberFormatException nfe) {
			return 0;
		}
	}
	
    public static String getMotorAction(Activity activity, String hiveId, int motorIndex) {
    	SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
    	return SP.getString(uniqueIdentifier(MOTOR_ACTION_PROPERTY, hiveId, motorIndex), DEFAULT_MOTOR_ACTION);
    }
    
	public static void resetMotorProperty(Activity activity, String hiveId, int motorIndex) {
		setMotorProperty(activity, hiveId, motorIndex, DEFAULT_MOTOR_ACTION, DEFAULT_MOTOR_TIMESTAMP);
	}
	
	public static void setMotorProperty(Activity activity, String hiveId, int motorIndex, String action, long date) {
		setMotorProperty(activity, hiveId, motorIndex, action, date==0 ? DEFAULT_MOTOR_TIMESTAMP : Long.toString(date));
	}
	
	public static void setMotorProperty(Activity activity, String hiveId, int motorIndex, String action, String date) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		if (!SP.getString(uniqueIdentifier(MOTOR_ACTION_PROPERTY, hiveId, motorIndex), DEFAULT_MOTOR_ACTION).equals(action) ||
			!SP.getString(uniqueIdentifier(MOTOR_TIMESTAMP_PROPERTY, hiveId, motorIndex), DEFAULT_MOTOR_TIMESTAMP).equals(date)) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(uniqueIdentifier(MOTOR_ACTION_PROPERTY, hiveId, motorIndex), action);
			editor.putString(uniqueIdentifier(MOTOR_TIMESTAMP_PROPERTY, hiveId, motorIndex), date);
			editor.commit();
		}
	}
}

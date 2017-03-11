package com.jfc.apps.hive;

import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

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
import com.jfc.misc.prop.PropertyBase;
import com.jfc.misc.prop.StepsPerRevolutionProperty;
import com.jfc.misc.prop.ThreadsPerMeterProperty;
import com.jfc.srvc.cloud.PollSensorBackground;
import com.jfc.srvc.cloud.PollSensorBackground.OnSaveValue;
import com.jfc.util.misc.DbAlertHandler;
import com.jfc.util.misc.SplashyText;

public class MotorProperty extends PropertyBase {
	private static final String TAG = MotorProperty.class.getName();

	private static final String MOTOR_ACTION_PROPERTY = "MOTOR_ACTION_PROPERTY";
	private static final String MOTOR_TIMESTAMP_PROPERTY = "MOTOR_TIMESTAMP_PROPERTY";
	private static final String MOTOR_ACTION_STOPPED = "stopped";
	private static final String MOTOR_ACTION_MOVING = "moving";
	private static final String DEFAULT_MOTOR_ACTION = MOTOR_ACTION_STOPPED;
	private static final String DEFAULT_MOTOR_TIMESTAMP = "0";
	
	private AlertDialog mAlert;
	private int mMotorIndex;
	private DbAlertHandler mDbAlertHandler;
	
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
		super(_activity, _hiveId, _valueTV, button, _timestampTV);
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

	
	private void showDialog() {
		AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
		
		builder.setIcon(R.drawable.ic_hive);
		builder.setView(R.layout.motor_dialog);
		builder.setTitle("Motor "+Integer.toString(mMotorIndex+1)+" distance (mm)");
		builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
        	@Override
        	public void onClick(DialogInterface dialog, int which) {
				EditText et = (EditText) mAlert.findViewById(R.id.textValue);
				String linearDistanceMillimetersStr = et.getText().toString();
				double linearDistanceMillimeters = Long.parseLong(linearDistanceMillimetersStr);
				long steps = linearDistanceToSteps(mActivity, linearDistanceMillimeters/1000.0);

				if (steps != 0) {
					String sensor = "motor"+Integer.toString(mMotorIndex)+"-target";
					postToDb(sensor, Long.toString(steps));
					String action = "moving"+(steps > 0 ? "+":"")+Long.toString(steps);
					long timestamp_s = System.currentTimeMillis()/1000;
					String timestamp = Long.toString(timestamp_s);
					MotorProperty.setMotorProperty(mActivity, mHiveId, mMotorIndex, action, timestamp);
					display();
					startPolling(Math.abs(steps)/StepsPerRevolutionProperty.getStepsPerSecond(mActivity));
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
	
	public void pollCloud() {
		String dbUrl = DbCredentialsProperty.getCouchLogDbUrl(mActivity);
		PollSensorBackground.ResultCallback onCompletion = 
			getMotorOnCompletion(mValueTV.getId(), mTimestampTV.getId(), new OnSaveValue() {
				@Override
				public void save(Activity activity, String objId, String value, long timestamp_s) {
					if (value.startsWith(MOTOR_ACTION_MOVING)) {
						// determine if the value should be ignored
						long steps = Long.parseLong(value.substring(MOTOR_ACTION_MOVING.length()));
						long shouldHaveTaken_s = Math.abs(steps)/StepsPerRevolutionProperty.getStepsPerSecond(mActivity);
						long shouldHaveFinishedBy_s = timestamp_s + shouldHaveTaken_s;
						long now_s = System.currentTimeMillis()/1000;
						if (now_s > shouldHaveFinishedBy_s+shouldHaveTaken_s) { // give it twice as long as it should have
							// ignore it!
							value = MOTOR_ACTION_STOPPED;
						}
					}
					MotorProperty.setMotorProperty(mActivity, mHiveId, mMotorIndex, value, timestamp_s);
				}
			});
		new PollSensorBackground(dbUrl, PollSensorBackground.createQuery(mHiveId, "motor"+Integer.toString(mMotorIndex)), 
				onCompletion).execute();
	}

	
	private void inc() {
		EditText tv = (EditText) mAlert.findViewById(R.id.textValue);
		String valueStr = tv.getText().toString();
		int n = Integer.parseInt(valueStr)+1;
		tv.setText(Integer.toString(n));
		SplashyText.highlightModifiedField(mActivity, tv);
	}
	
	private void inc100() {
		EditText tv = (EditText) mAlert.findViewById(R.id.textValue);
		String valueStr = tv.getText().toString();
		int n = Integer.parseInt(valueStr)+100;
		tv.setText(Integer.toString(n));
		SplashyText.highlightModifiedField(mActivity, tv);
	}
	
	private void dec() {
		EditText tv = (EditText) mAlert.findViewById(R.id.textValue);
		String valueStr = tv.getText().toString();
		int n = Integer.parseInt(valueStr)-1;
		tv.setText(Integer.toString(n));
		SplashyText.highlightModifiedField(mActivity, tv);
	}
	
	private void dec100() {
		EditText tv = (EditText) mAlert.findViewById(R.id.textValue);
		String valueStr = tv.getText().toString();
		int n = Integer.parseInt(valueStr)-100;
		tv.setText(Integer.toString(n));
		SplashyText.highlightModifiedField(mActivity, tv);
	}

	
	private AtomicBoolean mStopPoller = new AtomicBoolean(false);
	private AtomicInteger mPollCounter = new AtomicInteger(0);
	private Runnable mPoller = null;
	private Thread mPollerThread = null;
	private void startPolling(final double startPollingIn_s) {
		if (mPoller == null) {
			mPoller = new Runnable() {
				@Override
				public void run() {
					// first wait a while -- since I know how long the motor should run, no use polling until around when it 
					// should be done

					// 95ms*10==950ms -- nearly 1s
					// 95ms*(10*startPollingIn_s) == desired wait before polling
					mPollCounter.set(0);
					while (!mStopPoller.get() && (mPollCounter.getAndIncrement() < 10*startPollingIn_s)) {
						try {
							Thread.sleep(95);
						} catch (InterruptedException e) {
							// TODO Auto-generated catch block
							e.printStackTrace();
						}
					}

					// now poll the db once every 3s, but give up if it goes a full minute
					long started = System.currentTimeMillis();
					mPollCounter.set(0);
					boolean done = false;
					while (!done && !mStopPoller.get()) {
						try {
							Thread.sleep(95);
						} catch (InterruptedException e) {
							// TODO Auto-generated catch block
							e.printStackTrace();
						}
						
						// 95ms*5*10==4750ms
						if (mPollCounter.getAndIncrement() >= 3*10) {
							mPollCounter.set(0);

							pollCloud();
							done = getMotorAction(mActivity, mHiveId, mMotorIndex).equals(MOTOR_ACTION_STOPPED) ||
									   (System.currentTimeMillis()-started > 60000);
						}
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
		long timestampMillis = getMotorTimestamp(mActivity, mHiveId, mMotorIndex)*1000;

		String action = getMotorAction(mActivity, mHiveId, mMotorIndex);
		boolean isStopped = !action.startsWith(MOTOR_ACTION_MOVING);
		if (!isStopped) {
			// determine if the property value should be ignored
			long steps = Long.parseLong(action.substring(MOTOR_ACTION_MOVING.length()));
			long shouldHaveTaken_s = Math.abs(steps)/StepsPerRevolutionProperty.getStepsPerSecond(mActivity);
			long timestamp_s = timestampMillis/1000;
			long shouldHaveFinishedBy_s = timestamp_s + shouldHaveTaken_s;
			long now_s = System.currentTimeMillis()/1000;
			if (now_s > shouldHaveFinishedBy_s+shouldHaveTaken_s) { // give it twice as long as it should have
				// ignore it!
				isStopped = true;
			}
		}
		
		if (isStopped) {
			action = MOTOR_ACTION_STOPPED;
			mButton.setImageResource(R.drawable.ic_rarrow);
	    	mButton.setEnabled(true);
		} else {
			action = MOTOR_ACTION_MOVING;
			mButton.setImageResource(R.drawable.ic_rarrow_disabled);
	    	mButton.setEnabled(false);
		}
		long timestamp_s = timestampMillis / 1000;
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
                        display();
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
	
	private static void setMotorProperty(Activity activity, String hiveId, int motorIndex, String action, String date) {
		{
			SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
			if (!SP.getString(uniqueIdentifier(MOTOR_ACTION_PROPERTY, hiveId, motorIndex), DEFAULT_MOTOR_ACTION).equals(action)) {
				SharedPreferences.Editor editor = SP.edit();
				editor.putString(uniqueIdentifier(MOTOR_ACTION_PROPERTY, hiveId, motorIndex), action);
				editor.commit();
			}
		}
		
		{
			SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
			if (!SP.getString(uniqueIdentifier(MOTOR_TIMESTAMP_PROPERTY, hiveId, motorIndex), DEFAULT_MOTOR_TIMESTAMP).equals(date)) {
				SharedPreferences.Editor editor = SP.edit();
				editor.putString(uniqueIdentifier(MOTOR_TIMESTAMP_PROPERTY, hiveId, motorIndex), date);
				editor.commit();
			}
		}
	}
}

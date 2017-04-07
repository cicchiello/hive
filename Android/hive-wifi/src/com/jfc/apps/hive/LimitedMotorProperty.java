package com.jfc.apps.hive;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.TextView;

import com.jfc.misc.prop.StepsPerSecondProperty;
import com.jfc.util.misc.SplashyText;

public class LimitedMotorProperty extends MotorProperty {
	private static final String TAG = LimitedMotorProperty.class.getName();

	private static final String MOTOR_AT_POS_LIMIT = "@pos-limit";
	private static final String MOTOR_AT_NEG_LIMIT = "@neg-limit";
	private static final String MOTOR_TO_POS_LIMIT = "pos-limit";
	private static final String MOTOR_TO_NEG_LIMIT = "neg-limit";
	
	public LimitedMotorProperty(Activity _activity, String _hiveId, int _motorIndex, TextView _valueTV, ImageButton button, TextView _timestampTV) {
		super(_activity, _hiveId, _motorIndex, _valueTV, button, _timestampTV);
	}

	@Override
	protected void showDialog() {
		AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
		
		builder.setIcon(R.drawable.ic_hive);
		builder.setView(R.layout.limit_motor_dialog);
		builder.setTitle("Motor "+Integer.toString(mMotorIndex+1)+" distance (mm)");
		builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
        	@Override
        	public void onClick(DialogInterface dialog, int which) {
				EditText et = (EditText) mAlert.findViewById(R.id.textValue);
				double linearDistanceMillimeters = 0;
				if (et.getText().toString().equals(MOTOR_TO_POS_LIMIT)) {
					linearDistanceMillimeters = 1000;
				} else if (et.getText().toString().equals(MOTOR_TO_NEG_LIMIT)) {
					linearDistanceMillimeters = -1000;
				} else {
					linearDistanceMillimeters = Long.parseLong(et.getText().toString());
				}
				long steps = linearDistanceToSteps(mActivity, linearDistanceMillimeters/1000.0);

				if (steps != 0) {
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

		String action = getMotorAction(mActivity, mHiveId, mMotorIndex);
		
        Button plusButton = (Button) mAlert.findViewById(R.id.plus);
        Button plusLimitButton = (Button) mAlert.findViewById(R.id.plusLimit);
        Button minusButton = (Button) mAlert.findViewById(R.id.minus);
        Button minusLimitButton = (Button) mAlert.findViewById(R.id.minusLimit);
        
        plusButton.setOnClickListener(new View.OnClickListener() {public void onClick(View v) {inc();}});
        minusButton.setOnClickListener(new View.OnClickListener() {public void onClick(View v) {dec();}});
        plusLimitButton.setOnClickListener(new View.OnClickListener() {public void onClick(View v) {posLimit();}});
        minusLimitButton.setOnClickListener(new View.OnClickListener() {public void onClick(View v) {negLimit();}});
        
        plusButton.setEnabled(!action.equals(MOTOR_AT_POS_LIMIT));
        plusLimitButton.setEnabled(!action.equals(MOTOR_AT_POS_LIMIT));
        minusButton.setEnabled(!action.equals(MOTOR_AT_NEG_LIMIT));
        minusLimitButton.setEnabled(!action.equals(MOTOR_AT_NEG_LIMIT));
	}


	private void disableAllDirectionButtons() {
        Button plusButton = (Button) mAlert.findViewById(R.id.plus);
        Button minusButton = (Button) mAlert.findViewById(R.id.minus);
        Button plusLimitButton = (Button) mAlert.findViewById(R.id.plusLimit);
        Button minusLimitButton = (Button) mAlert.findViewById(R.id.minusLimit);
        plusButton.setEnabled(false);
        plusLimitButton.setEnabled(false);
        minusButton.setEnabled(false);
        minusLimitButton.setEnabled(false);
	}

	protected void inc() {
		EditText tv = (EditText) mAlert.findViewById(R.id.textValue);
		String valueStr = tv.getText().toString();
		int n = Integer.parseInt(valueStr)+1;
		tv.setText(Integer.toString(n));
		SplashyText.highlightModifiedField(mActivity, tv);
        mAlert.findViewById(R.id.minus).setEnabled(true);
        if (n == 0) {
        	// handle the case where that puts us back at pos-limit
        	String action = getMotorAction(mActivity, mHiveId, mMotorIndex);
            mAlert.findViewById(R.id.plus).setEnabled(!action.equals(MOTOR_AT_POS_LIMIT));
            mAlert.findViewById(R.id.plusLimit).setEnabled(!action.equals(MOTOR_AT_POS_LIMIT));
        }
	}
	
	protected void dec() {
		EditText tv = (EditText) mAlert.findViewById(R.id.textValue);
		String valueStr = tv.getText().toString();
		int n = Integer.parseInt(valueStr)-1;
		tv.setText(Integer.toString(n));
		SplashyText.highlightModifiedField(mActivity, tv);
        mAlert.findViewById(R.id.plus).setEnabled(true);
        if (n == 0) {
        	// handle the case where that puts us back at neg-limit
        	String action = getMotorAction(mActivity, mHiveId, mMotorIndex);
            mAlert.findViewById(R.id.minus).setEnabled(!action.equals(MOTOR_AT_NEG_LIMIT));
            mAlert.findViewById(R.id.minusLimit).setEnabled(!action.equals(MOTOR_AT_NEG_LIMIT));
        }
	}
	
	private void posLimit() {
		EditText tv = (EditText) mAlert.findViewById(R.id.textValue);
		tv.setText(MOTOR_TO_POS_LIMIT);
		SplashyText.highlightModifiedField(mActivity, tv);
		tv.setEnabled(false);
		disableAllDirectionButtons();
	}
	
	private void negLimit() {
		EditText tv = (EditText) mAlert.findViewById(R.id.textValue);
		tv.setText(MOTOR_TO_NEG_LIMIT);
		SplashyText.highlightModifiedField(mActivity, tv);
		tv.setEnabled(false);
		disableAllDirectionButtons();
	}

	
	// do whatever is necessary to display the current state in the ParentView
	protected void display() {
		long timestamp_s = getMotorTimestamp(mActivity, mHiveId, mMotorIndex);

		String action = getMotorAction(mActivity, mHiveId, mMotorIndex);
		boolean isStopped = action.equals(MOTOR_ACTION_STOPPED) || 
							action.equals(MOTOR_AT_NEG_LIMIT) || 
							action.equals(MOTOR_AT_POS_LIMIT);
		if (!isStopped) {
			// determine if the property value should be ignored
			long steps = Long.parseLong(action.substring(MOTOR_ACTION_MOVING.length()));
			long shouldHaveTaken_s = Math.abs(steps)/StepsPerSecondProperty.getStepsPerSecond(mActivity, mHiveId);
			long shouldHaveFinishedBy_s = timestamp_s + shouldHaveTaken_s;
			long now_s = System.currentTimeMillis()/1000;
			if (now_s > shouldHaveFinishedBy_s+shouldHaveTaken_s) { // give it twice as long as it should have
				// ignore it!
				isStopped = true;
			}
		}
		
		if (isStopped) {
			mButton.setImageResource(R.drawable.ic_rarrow);
	    	mButton.setEnabled(true);
		} else {
			action = MOTOR_ACTION_MOVING;
			mButton.setImageResource(R.drawable.ic_rarrow_disabled);
	    	mButton.setEnabled(false);
		}
		HiveEnv.setValueWithSplash(mActivity, mValueTV.getId(), mTimestampTV.getId(), action, true, timestamp_s);
	}

}

package com.jfc.apps.hive;

import java.util.Calendar;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

import com.jfc.misc.prop.ActiveHiveProperty;
import com.jfc.misc.prop.DbCredentialsProperty;
import com.jfc.misc.prop.UptimeProperty;
import com.jfc.srvc.ble2cld.BluetoothPipeSrvc;
import com.jfc.srvc.ble2cld.PollSensorBackground;
import com.jfc.util.misc.DbAlertHandler;
import com.jfc.util.misc.SplashyText;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.text.format.DateFormat;
import android.text.format.DateUtils;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.ImageButton;
import android.widget.TextView;

public class MainActivity extends Activity {
	private static final String TAG = MainActivity.class.getSimpleName();
	
    private static final int POLL_RATE = 600;
    
    private static final boolean DEBUG = HiveEnv.DEBUG && !HiveEnv.RELEASE_TEST;

    // db polling -- necessary until/unless we have an active notification upon db change
    // the following vars are managed by the startPolling() and cancelPolling() functions
	private AtomicBoolean mStopPoller = new AtomicBoolean(false);
	private AtomicInteger mPollCounter = new AtomicInteger(0);
	private Thread mPollerThread = null;
	private Runnable mPoller = null;
	private MotorProperty m0, m1, m2;
	private AlertDialog mAlert;
	
	private DbAlertHandler mDbAlert = null;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		setContentView(DEBUG ? R.layout.hive_main_debug : R.layout.hive_main);
	
		// set initial value(s)
		setInitialValues();

		mDbAlert = new DbAlertHandler();
		
    	startPolling();
    	
    	BluetoothPipeSrvc.startBlePipes(this);
    	
    	boolean haveActiveHive = ActiveHiveProperty.isActiveHivePropertyDefined(this);
    	String title = getString(R.string.app_name) + (haveActiveHive ? ": "+ActiveHiveProperty.getActiveHiveProperty(this) : "");
    	setTitle(title);
	}

	private void setValue(int valueResid, int timestampResid, String value, boolean isTimestampDefined, long timestamp) {
		setValueImplementation(valueResid, timestampResid, value, isTimestampDefined, timestamp, false);
	}

	private void setValueWithSplash(int valueResid, int timestampResid, String value, boolean isTimestampDefined, long timestamp) {
		setValueImplementation(valueResid, timestampResid, value, isTimestampDefined, timestamp, true);
	}

	private void setInitialValues() {
		if (DEBUG) {
			((TextView) findViewById(R.id.cpuTempText)).setText("?");
			((TextView) findViewById(R.id.cpuTempTimestampText)).setText("?");
		}
		
		((TextView) findViewById(R.id.tempText)).setText("?");
		((TextView) findViewById(R.id.tempTimestampText)).setText("?");
		
		((TextView) findViewById(R.id.humidText)).setText("?");
		((TextView) findViewById(R.id.humidTimestampText)).setText("?");

		m0 = new MotorProperty(this, 0, 
							   (TextView) findViewById(R.id.motor0Text), 
							   (ImageButton) findViewById(R.id.selectMotor0Button),
							   (TextView) findViewById(R.id.motor0TimestampText));
		
		m1 = new MotorProperty(this, 1, 
							   (TextView) findViewById(R.id.motor1Text), 
							   (ImageButton) findViewById(R.id.selectMotor1Button),
							   (TextView) findViewById(R.id.motor1TimestampText));

		m2 = new MotorProperty(this, 2, 
							   (TextView) findViewById(R.id.motor2Text), 
							   (ImageButton) findViewById(R.id.selectMotor2Button),
							   (TextView) findViewById(R.id.motor2TimestampText));

		try {
			if (DEBUG) {
				setValue(R.id.cpuTempText, R.id.cpuTempTimestampText,
						 MCUTempProperty.getMCUTempValue(this), 
						 MCUTempProperty.isMCUTempPropertyDefined(this), 
						 MCUTempProperty.getMCUTempDate(this));
			}
			
			setValue(R.id.tempText, R.id.tempTimestampText,
					 TempProperty.getTempValue(this), 
					 TempProperty.isTempPropertyDefined(this), 
					 TempProperty.getTempDate(this));
			
			setValue(R.id.humidText, R.id.humidTimestampText,
					 HumidProperty.getHumidValue(this), 
					 HumidProperty.isHumidPropertyDefined(this), 
					 HumidProperty.getHumidDate(this));
			
			setValue(R.id.motor0Text, R.id.motor0TimestampText,
					 MotorProperty.getMotorValue(this, 0), 
					 MotorProperty.isMotorPropertyDefined(this, 0), 
					 MotorProperty.getMotorDate(this, 0));
			
			setValue(R.id.motor1Text, R.id.motor1TimestampText,
					 MotorProperty.getMotorValue(this, 1), 
					 MotorProperty.isMotorPropertyDefined(this, 1), 
					 MotorProperty.getMotorDate(this, 1));
			
			setValue(R.id.motor2Text, R.id.motor2TimestampText,
					 MotorProperty.getMotorValue(this, 2), 
					 MotorProperty.isMotorPropertyDefined(this, 2), 
					 MotorProperty.getMotorDate(this, 2));
						
			setValue(R.id.beecntText, R.id.beecntTimestampText,
					 BeeCntProperty.getBeeCntValue(this), 
					 BeeCntProperty.isBeeCntPropertyDefined(this), 
					 BeeCntProperty.getBeeCntDate(this));
						
			setValue(R.id.hiveUptimeText, R.id.hiveUptimeTimestampText,
					 "foo", 
					 UptimeProperty.isUptimePropertyDefined(this), 
					 UptimeProperty.getUptimeProperty(this));
			
			ImageButton uptimeButton = (ImageButton) findViewById(R.id.selectHiveUptimeButton);
			View.OnClickListener ocl = new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					AlertDialog.Builder builder = new AlertDialog.Builder(MainActivity.this);
					
					builder.setIcon(R.drawable.ic_hive);
					builder.setView(R.layout.uptime_dialog);
					builder.setTitle(R.string.uptime_dialog_title);
			        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
			            @Override
			            public void onClick(DialogInterface dialog, int which) {mAlert.dismiss(); mAlert = null;}
			        });
			        mAlert = builder.show();

			        String upsinceValueStr = "unknown";
			        if (UptimeProperty.isUptimePropertyDefined(MainActivity.this)) {
						Calendar cal = Calendar.getInstance(Locale.ENGLISH);
						cal.setTimeInMillis(1000*UptimeProperty.getUptimeProperty(MainActivity.this));
						upsinceValueStr = DateFormat.format("dd-MMM-yy HH:mm",  cal).toString();
			        }
			        
			        TextView upsinceTv = (TextView) mAlert.findViewById(R.id.upsince_text);
			        TextView statusTv = (TextView) mAlert.findViewById(R.id.current_status_text);
			        upsinceTv.setText(upsinceValueStr);
			        boolean isConnected = BluetoothPipeSrvc.isConnected(MainActivity.this);
			        statusTv.setText(isConnected ? "Connected" : "Unknown");
				}
			};
			uptimeButton.setOnClickListener(ocl);
			
		} catch (Exception ex) {
			Log.e(TAG, ex.getLocalizedMessage());
		}
	}

	private String createQuery(String hiveId, String sensor) {
		String rangeStartKeyClause = "[\"" + hiveId + "\",\""+sensor+"\",\"99999999\"]";
		String rangeEndKeyClause = "[\"" + hiveId + "\",\""+sensor+"\",\"00000000\"]";
		String query = "_design/SensorLog/_view/by-hive-sensor?endKey=" + rangeEndKeyClause + "&startkey=" + rangeStartKeyClause + "&descending=true&limit=1";
		return query;
	}
	
	interface OnSaveValue {
		public void save(Activity activity, String value, long timestamp);
	};
	
	
	private PollSensorBackground.ResultCallback getSensorOnCompletion(final int valueResid, final int timestampResid, final OnSaveValue saver) {
		PollSensorBackground.ResultCallback onCompletion = new PollSensorBackground.ResultCallback() {
			@Override
	    	public void report(String sensorType, final String timestampStr, final String valueStr) {
				runOnUiThread(new Runnable() {
					@Override
					public void run() {
						long timestampSeconds = Long.parseLong(timestampStr);
						long timestampMillis = timestampSeconds*1000;
						setValueWithSplash(valueResid, timestampResid, valueStr, true, timestampMillis);
						saver.save(MainActivity.this, valueStr, timestampSeconds);
					}
				});
			}
			
			@Override
			public void error(final String msg) {
				String errMsg = "Attempt to get Sensor state failed with this message: "+msg;
				mDbAlert.informDbInaccessible(MainActivity.this, errMsg, valueResid);
			}

			@Override
			public void dbAccessibleError(final String msg) {
				mDbAlert.informDbInaccessible(MainActivity.this, getString(R.string.db_inaccessible), valueResid);
			}
		};
		return onCompletion;
	}
	
	private PollSensorBackground.ResultCallback getMotorOnCompletion(final int valueResid, final int timestampResid, final OnSaveValue saver) {
		PollSensorBackground.ResultCallback onCompletion = new PollSensorBackground.ResultCallback() {
			@Override
	    	public void report(String sensorType, final String timestampStr, final String stepsStr) {
				runOnUiThread(new Runnable() {
					@Override
					public void run() {
						long timestampSeconds = Long.parseLong(timestampStr);
						long timestampMillis = timestampSeconds*1000;
						double linearDistanceMeters = MotorProperty.stepsToLinearDistance(MainActivity.this, Long.parseLong(stepsStr));
						double linearDistanceMillimeters = linearDistanceMeters*1000.0;
						String linearDistanceStr = Long.toString((long)(linearDistanceMillimeters+0.5));
						setValueWithSplash(valueResid, timestampResid, linearDistanceStr, true, timestampMillis);
						saver.save(MainActivity.this, linearDistanceStr, timestampSeconds);
					}
				});
			}
			
			@Override
			public void error(final String msg) {
				String errMsg = "Attempt to get Motor location failed with this message: "+msg;
				mDbAlert.informDbInaccessible(MainActivity.this, errMsg, valueResid);
			}

			@Override
			public void dbAccessibleError(final String msg) {
				mDbAlert.informDbInaccessible(MainActivity.this, msg, valueResid);
			}
		};
		return onCompletion;
	}
	
	
	private void startPolling() {
		synchronized (this) {
			if (mPoller == null) {
				mPoller = new Runnable() {
					@Override
					public void run() {
						// check about once a minute whenever this activity is running (it doesn't get
						// updated more frequently)
						mPollCounter.set(POLL_RATE-10); // start nearly full scale so that first poll is soon -- subsequent's will be spaced regularly
						while (!mStopPoller.get()) {
							try {
								Thread.sleep(95);
							} catch (InterruptedException e) {
								// TODO Auto-generated catch block
								e.printStackTrace();
							}
							
							// 95ms*600==57000ms
							if (mPollCounter.getAndIncrement() >= POLL_RATE) {
								mPollCounter.set(0);
								
								String ActiveHive = ActiveHiveProperty.getActiveHiveProperty(MainActivity.this);
								final String HiveId = HiveEnv.getHiveAddress(MainActivity.this, ActiveHive);
								boolean haveHiveId = HiveId != null;
								
								if (haveHiveId) {
									PollSensorBackground.ResultCallback onCompletion;
									String cloudantUser = DbCredentialsProperty.getCloudantUser(MainActivity.this);
									String dbName = DbCredentialsProperty.getDbName(MainActivity.this);
									String dbUrl = DbCredentialsProperty.CouchDbUrl(cloudantUser, dbName);

									if (DEBUG) {
										onCompletion = getSensorOnCompletion(R.id.cpuTempText, R.id.cpuTempTimestampText, new OnSaveValue() {
															@Override
															public void save(Activity activity, String value, long timestamp) {
																MCUTempProperty.setMCUTempProperty(MainActivity.this, value, timestamp);
															}
														});
							            new PollSensorBackground(dbUrl, createQuery(HiveId.replace(':', '-'), "cputemp"), onCompletion).execute();
									}
									
									onCompletion = 
											getSensorOnCompletion(R.id.tempText, R.id.tempTimestampText, new OnSaveValue() {
												@Override
												public void save(Activity activity, String value, long timestamp) {
											        TempProperty.setTempProperty(MainActivity.this, value, timestamp);
												}
											});
						            new PollSensorBackground(dbUrl, createQuery(HiveId.replace(':', '-'), "temp"), onCompletion).execute();
						            
									onCompletion = 
											getSensorOnCompletion(R.id.humidText, R.id.humidTimestampText, new OnSaveValue() {
												@Override
												public void save(Activity activity, String value, long timestamp) {
											        HumidProperty.setHumidProperty(MainActivity.this, value, timestamp);
												}
											});
						            new PollSensorBackground(dbUrl, createQuery(HiveId.replace(':', '-'), "humid"), onCompletion).execute();
						            
									onCompletion = getSensorOnCompletion(R.id.beecntText, R.id.beecntTimestampText, new OnSaveValue() {
										@Override
										public void save(Activity activity, String value, long timestamp) {
											BeeCntProperty.setBeeCntProperty(MainActivity.this, value, timestamp);
										}
									});
									new PollSensorBackground(dbUrl, createQuery(HiveId.replace(':', '-'), "beecnt"), onCompletion).execute();

									onCompletion = 
											getMotorOnCompletion(R.id.motor0Text, R.id.motor0TimestampText, new OnSaveValue() {
												@Override
												public void save(Activity activity, String value, long timestamp) {
													MotorProperty.setMotorProperty(MainActivity.this, 0, value, timestamp);
												}
											});
						            new PollSensorBackground(dbUrl, createQuery(HiveId.replace(':', '-'), "motor0"), onCompletion).execute();

									onCompletion = 
											getMotorOnCompletion(R.id.motor1Text, R.id.motor1TimestampText, new OnSaveValue() {
												@Override
												public void save(Activity activity, String value, long timestamp) {
													MotorProperty.setMotorProperty(MainActivity.this, 1, value, timestamp);
												}
											});
						            new PollSensorBackground(dbUrl, createQuery(HiveId.replace(':', '-'), "motor1"), onCompletion).execute();
						            
									onCompletion = 
											getMotorOnCompletion(R.id.motor2Text, R.id.motor2TimestampText, new OnSaveValue() {
												@Override
												public void save(Activity activity, String value, long timestamp) {
													MotorProperty.setMotorProperty(MainActivity.this, 2, value, timestamp);
												}
											});
						            new PollSensorBackground(dbUrl, createQuery(HiveId.replace(':', '-'), "motor2"), onCompletion).execute();
						            
						    		if (UptimeProperty.isUptimePropertyDefined(MainActivity.this)) {
										final long uptimeMillis = UptimeProperty.getUptimeProperty(MainActivity.this)*1000;
										final CharSequence since = DateUtils.getRelativeTimeSpanString(uptimeMillis,
												System.currentTimeMillis(), DateUtils.MINUTE_IN_MILLIS, DateUtils.FORMAT_ABBREV_RELATIVE);
										runOnUiThread(new Runnable() {
											@Override
											public void run() {
									    		TextView uptimeTv = (TextView) findViewById(R.id.hiveUptimeText);
									    		if (!since.equals(uptimeTv.getText().toString())) {
									    			uptimeTv.setText(since);
													SplashyText.highlightModifiedField(MainActivity.this, uptimeTv);
									    		}
									    		TextView uptimeTimestampTv = (TextView) findViewById(R.id.hiveUptimeTimestampText);
												Calendar cal = Calendar.getInstance(Locale.ENGLISH);
												cal.setTimeInMillis(System.currentTimeMillis());
												final String timestampStr = DateFormat.format("dd-MMM-yy HH:mm",  cal).toString();
												if (!timestampStr.equals(uptimeTimestampTv.getText().toString())) {
													uptimeTimestampTv.setText(timestampStr);
													SplashyText.highlightModifiedField(MainActivity.this, uptimeTimestampTv);
												}
											}
										});
						    		}
								}
							}
						}
					}
				};
				mStopPoller.set(false);
				mPollerThread = new Thread(mPoller, "CouchPoller");
				mPollerThread.start();
			}
		}
	}
	
	private void cancelPoller() {
		mStopPoller.set(true);
		mPollerThread = null;
		mPoller = null;
	}
	
	@Override
	protected void onPause() {
		cancelPoller();
		
		mDbAlert.onPause(this);
		
		super.onPause();
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		
		setInitialValues();
		startPolling();
		
    	boolean haveActiveHive = ActiveHiveProperty.isActiveHivePropertyDefined(this);
    	String title = getString(R.string.app_name) + (haveActiveHive ? ": "+ActiveHiveProperty.getActiveHiveProperty(this) : "");
    	setTitle(title);
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		mPollCounter.set(POLL_RATE-10); // force the fetch to happen very soon
	}
	
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle action bar item clicks here. The action bar will
		// automatically handle clicks on the Home/Up button, so long
		// as you specify a parent activity in AndroidManifest.xml.
		int id = item.getItemId();
		if (id == R.id.action_settings) {
			onSettings();
			return true;
		}
		if (id == R.id.action_ble_settings) {
			onBleSettings();
			return true;
		}
		if (id == R.id.action_motor_settings) {
			onMotorSettings();
			return true;
		}
		return super.onOptionsItemSelected(item);
	}
	
	public void onSettings() {
		Intent intent = new Intent(this, HiveSettingsActivity.class);
		startActivityForResult(intent, 0);
	}
	
	public void onBleSettings() {
		Intent intent = new Intent(this, BleSettingsActivity.class);
		startActivityForResult(intent, 0);
	}
	
	public void onMotorSettings() {
		Intent intent = new Intent(this, MotorSettingsActivity.class);
		startActivityForResult(intent, 0);
	}
	
	private void setValueImplementation(int valueResid, int timestampResid, 
										String value, boolean isTimestampDefined, long timestamp, 
										boolean addSplash) 
	{
		TextView valueTv = (TextView) findViewById(valueResid);
		TextView timestampTv = (TextView) findViewById(timestampResid);
		boolean splashValue = !valueTv.getText().equals(value);
		valueTv.setText(value);
		boolean splashTimestamp = false;
		if (isTimestampDefined) {
			Calendar cal = Calendar.getInstance(Locale.ENGLISH);
			cal.setTimeInMillis(timestamp);
			String timestampStr = DateFormat.format("dd-MMM-yy HH:mm",  cal).toString();
			splashTimestamp = !timestampTv.getText().equals(timestampStr);
			timestampTv.setText(timestampStr);
		} else {
			splashTimestamp = !timestampTv.getText().equals("<unknown>");
			timestampTv.setText("<unknown>");
		}
		if (addSplash) {
			if (splashValue) 
				SplashyText.highlightModifiedField(this, valueTv);
			if (splashTimestamp) 
				SplashyText.highlightModifiedField(this, timestampTv);
		}
	}
	
}

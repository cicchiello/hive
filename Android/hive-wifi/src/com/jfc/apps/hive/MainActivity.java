package com.jfc.apps.hive;

import java.util.Calendar;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

import com.jfc.misc.prop.ActiveHiveProperty;
import com.jfc.misc.prop.AudioSampler;
import com.jfc.misc.prop.DbCredentialsProperty;
import com.jfc.misc.prop.LatchProperty;
import com.jfc.misc.prop.UptimeProperty;
import com.jfc.srvc.cloud.PollSensorBackground;
import com.jfc.util.misc.DbAlertHandler;
import com.jfc.util.misc.DialogUtils;
import com.jfc.util.misc.SplashyText;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.text.format.DateFormat;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.Toast;


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
	private LatchProperty latch;
	private UptimeProperty uptime;
	private AudioSampler audio;
	private AlertDialog mAlert;
	private boolean mInitialValuesSet = false;
	
	private DbAlertHandler mDbAlert = null;
	
	private final static int REQUEST_WRITE_EXTERNAL_STORAGE=1;
	
    private void requestPermission(){
            // permission has not been granted yet. Request it directly.
            requestPermissions(
                    new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE},
                    REQUEST_WRITE_EXTERNAL_STORAGE);
    }
    
    @Override
    public void onRequestPermissionsResult(int requestCode,String permissions[], int[] grantResults) {
        switch (requestCode) {
            case REQUEST_WRITE_EXTERNAL_STORAGE: {
                if (grantResults.length == 1 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    Toast.makeText(this, "permission success", Toast.LENGTH_SHORT).show();

                } else {
                    Toast.makeText(this, "permission failed", Toast.LENGTH_SHORT).show();
                    super.onRequestPermissionsResult(requestCode, permissions, grantResults);
                }
                return;
            }
        }
    }
    
    @Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		setContentView(DEBUG ? R.layout.hive_main_debug : R.layout.hive_main);
	
		// set initial value(s)
		setInitialValues();
		mInitialValuesSet = true;

		mDbAlert = new DbAlertHandler();
		
    	startPolling();
    	
    	String title = getString(R.string.app_name) + ": "+ActiveHiveProperty.getActiveHiveName(this);
    	setTitle(title);
    	
    }

	private void requestPermission(MainActivity mainActivity) {
		// TODO Auto-generated method stub
		
	}

	private void setValue(int valueResid, int timestampResid, String value, boolean isTimestampDefined, long timestamp) {
		setValueImplementation(valueResid, timestampResid, value, isTimestampDefined, timestamp, false);
	}

	private void setValueWithSplash(int valueResid, int timestampResid, String value, boolean isTimestampDefined, long timestamp) {
		setValueImplementation(valueResid, timestampResid, value, isTimestampDefined, timestamp, true);
	}

	private void setInitialValues() {
		int hiveIndex = ActiveHiveProperty.getActiveHiveIndex(this);
		
		if (DEBUG) {
			((TextView) findViewById(R.id.cpuTempText)).setText("?");
			((TextView) findViewById(R.id.cpuTempTimestampText)).setText("?");
		}
		
		((TextView) findViewById(R.id.tempText)).setText("?");
		((TextView) findViewById(R.id.tempTimestampText)).setText("?");
		
		((TextView) findViewById(R.id.humidText)).setText("?");
		((TextView) findViewById(R.id.humidTimestampText)).setText("?");

		latch = new LatchProperty(this, (TextView) findViewById(R.id.latch_text), 
								  (TextView) findViewById(R.id.latchTimestampText));
		
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

		
		uptime = new UptimeProperty(this,
									(TextView) findViewById(R.id.hiveUptimeText), 
									(ImageButton) findViewById(R.id.selectHiveUptimeButton),
									(TextView) findViewById(R.id.hiveUptimeTimestampText));
		
		if (DEBUG) {
			audio = new AudioSampler(this, (ImageButton) findViewById(R.id.audioSampleButton));
			
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
				 "TBD", 
				 UptimeProperty.isUptimePropertyDefined(this, hiveIndex), 
				 UptimeProperty.getUptime(this, hiveIndex));
		UptimeProperty.display(this, R.id.hiveUptimeText, R.id.hiveUptimeTimestampText);
	}

	private String createQuery(String hiveId, String sensor) {
		String rangeStartKeyClause = "[\"" + hiveId + "\",\""+sensor+"\",\"9999999999\"]";
		String rangeEndKeyClause = "[\"" + hiveId + "\",\""+sensor+"\",\"0000000000\"]";
		String query = "_design/SensorLog/_view/by-hive-sensor?endKey=" + rangeEndKeyClause + "&startkey=" + rangeStartKeyClause + "&descending=true&limit=1";
		return query;
	}
	
	interface OnSaveValue {
		public void save(Activity activity, String value, long timestamp);
	};
	
	
	private PollSensorBackground.ResultCallback getSensorOnCompletion(final String sensorName, 
																	  final int valueResid, final int timestampResid, 
																	  final OnSaveValue saver) {
		PollSensorBackground.ResultCallback onCompletion = new PollSensorBackground.ResultCallback() {
			@Override
	    	public void report(final String sensorType, final String timestampStr, final String valueStr) {
				runOnUiThread(new Runnable() {
					@Override
					public void run() {
						if (sensorName.equals(sensorType)) {
							long timestampSeconds = Long.parseLong(timestampStr);
							long timestampMillis = timestampSeconds*1000;
							setValueWithSplash(valueResid, timestampResid, valueStr, true, timestampMillis);
							saver.save(MainActivity.this, valueStr, timestampSeconds);
						}
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
								
								String ActiveHive = ActiveHiveProperty.getActiveHiveName(MainActivity.this);
								final String HiveId = HiveEnv.getHiveAddress(MainActivity.this, ActiveHive);
								final int hiveIndex = ActiveHiveProperty.getActiveHiveIndex(MainActivity.this);
								boolean haveHiveId = HiveId != null;
								
								if (haveHiveId) {
									PollSensorBackground.ResultCallback onCompletion;
									String dbUrl = DbCredentialsProperty.getCouchLogDbUrl(MainActivity.this);

									if (DEBUG) {
										onCompletion = getSensorOnCompletion("cputemp", R.id.cpuTempText, R.id.cpuTempTimestampText, new OnSaveValue() {
															@Override
															public void save(Activity activity, String value, long timestamp) {
																MCUTempProperty.setMCUTempProperty(MainActivity.this, value, timestamp);
															}
														});
							            new PollSensorBackground(dbUrl, createQuery(HiveId, "cputemp"), onCompletion).execute();
									}
									
									onCompletion = 
											getSensorOnCompletion("temp", R.id.tempText, R.id.tempTimestampText, new OnSaveValue() {
												@Override
												public void save(Activity activity, String value, long timestamp) {
											        TempProperty.setTempProperty(MainActivity.this, value, timestamp);
												}
											});
						            new PollSensorBackground(dbUrl, createQuery(HiveId, "temp"), onCompletion).execute();
						            
									onCompletion = 
											getSensorOnCompletion("humid", R.id.humidText, R.id.humidTimestampText, new OnSaveValue() {
												@Override
												public void save(Activity activity, String value, long timestamp) {
											        HumidProperty.setHumidProperty(MainActivity.this, value, timestamp);
												}
											});
						            new PollSensorBackground(dbUrl, createQuery(HiveId, "humid"), onCompletion).execute();
						            
									onCompletion = getSensorOnCompletion("beecnt", R.id.beecntText, R.id.beecntTimestampText, new OnSaveValue() {
										@Override
										public void save(Activity activity, String value, long timestamp) {
											BeeCntProperty.setBeeCntProperty(MainActivity.this, value, timestamp);
										}
									});
									new PollSensorBackground(dbUrl, createQuery(HiveId, "beecnt"), onCompletion).execute();

									onCompletion = getSensorOnCompletion("latch", R.id.latch_text, R.id.latchTimestampText, new OnSaveValue() {
										@Override
										public void save(Activity activity, String value, long timestamp) {
											LatchProperty.setLatchProperty(MainActivity.this, value, timestamp);
										}
									});
									new PollSensorBackground(dbUrl, createQuery(HiveId, "latch"), onCompletion).execute();

									onCompletion = 
											getMotorOnCompletion(R.id.motor0Text, R.id.motor0TimestampText, new OnSaveValue() {
												@Override
												public void save(Activity activity, String value, long timestamp) {
													MotorProperty.setMotorProperty(MainActivity.this, 0, value, timestamp);
												}
											});
						            new PollSensorBackground(dbUrl, createQuery(HiveId, "motor0"), onCompletion).execute();

									onCompletion = 
											getMotorOnCompletion(R.id.motor1Text, R.id.motor1TimestampText, new OnSaveValue() {
												@Override
												public void save(Activity activity, String value, long timestamp) {
													MotorProperty.setMotorProperty(MainActivity.this, 1, value, timestamp);
												}
											});
						            new PollSensorBackground(dbUrl, createQuery(HiveId, "motor1"), onCompletion).execute();
						            
									onCompletion = 
											getMotorOnCompletion(R.id.motor2Text, R.id.motor2TimestampText, new OnSaveValue() {
												@Override
												public void save(Activity activity, String value, long timestamp) {
													MotorProperty.setMotorProperty(MainActivity.this, 2, value, timestamp);
												}
											});
						            new PollSensorBackground(dbUrl, createQuery(HiveId, "motor2"), onCompletion).execute();
						            
									onCompletion = 
											getSensorOnCompletion("heartbeat", R.id.hiveUptimeText, R.id.hiveUptimeTimestampText, new OnSaveValue() {
												@Override
												public void save(Activity activity, String value, long timestamp) {
													UptimeProperty.setUptimeProperty(MainActivity.this, hiveIndex, Long.parseLong(value), timestamp);
													UptimeProperty.display(MainActivity.this, R.id.hiveUptimeText, R.id.hiveUptimeTimestampText);
												}
											});
						            new PollSensorBackground(dbUrl, createQuery(HiveId, "heartbeat"), onCompletion).execute();
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
		
		if (mAlert != null)
			mAlert.dismiss();
		
		if (audio != null) 
			audio.onPause();
		
		super.onPause();
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		
		if (!mInitialValuesSet)
			setInitialValues();
		
		startPolling();
		
    	String title = getString(R.string.app_name) + ": "+ActiveHiveProperty.getActiveHiveName(this);
    	setTitle(title);
    	
    	if (!ActiveHiveProperty.isActiveHivePropertyDefined(this)) {
    		Runnable onCancel = new Runnable() {
				@Override
				public void run() {
					mAlert.dismiss();
					mAlert = null;
					onSettings();
				}
			};
    		mAlert = DialogUtils.createAndShowErrorDialog(this, "You must establish a Hivewiz pairing first", 
    													  android.R.string.ok, onCancel);
    	}
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
	
	public void onWifiSettings() {
		Intent intent = new Intent(this, WifiSettingsActivity.class);
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

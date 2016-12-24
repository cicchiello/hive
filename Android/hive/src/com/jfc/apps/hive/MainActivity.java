package com.jfc.apps.hive;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicBoolean;

import com.example.hive.R;
import com.jfc.util.misc.SplashyText;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.content.Intent;
import android.os.Bundle;
import android.text.format.DateFormat;
import android.util.Log;
import android.util.Pair;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.TextView;

public class MainActivity extends Activity {
	private static final String TAG = MainActivity.class.getSimpleName();
	
    private static final String DbHost = "http://192.168.1.85";
    private static final int DbPort = 5984;
    private static final String Db = "hive-sensor-log";

    // db polling -- necessary until/unless we have an active notification upon db change
    // the following vars are managed by the startPolling() and cancelPolling() functions
	private AtomicBoolean mStopPoller = new AtomicBoolean(false);
	private Thread mPollerThread = null;
	private Runnable mPoller = null;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.hive_main);
	
		// set initial value(s)
		setInitialValues();

    	startPolling();
	}

	private void setValue(int valueResid, int timestampResid, String value, boolean isTimestampDefined, long timestamp) {
		setValueImplementation(valueResid, timestampResid, value, isTimestampDefined, timestamp, false);
	}

	private void setValueWithSplash(int valueResid, int timestampResid, String value, boolean isTimestampDefined, long timestamp) {
		setValueImplementation(valueResid, timestampResid, value, isTimestampDefined, timestamp, true);
	}

	private void setInitialValues() {
		setValue(R.id.cpuTempText, R.id.cpuTempTimestampText,
				 MCUTempProperty.getMCUTempValue(this), 
				 MCUTempProperty.isMCUTempPropertyDefined(this), 
				 MCUTempProperty.getMCUTempDate(this));
		
		setValue(R.id.tempText, R.id.tempTimestampText,
				 TempProperty.getTempValue(this), 
				 TempProperty.isTempPropertyDefined(this), 
				 TempProperty.getTempDate(this));
		
		setValue(R.id.humidText, R.id.humidTimestampText,
				 HumidProperty.getHumidValue(this), 
				 HumidProperty.isHumidPropertyDefined(this), 
				 HumidProperty.getHumidDate(this));
	}

	
	private String getHiveAddress(String hiveName) {
    	List<Pair<String,String>> existingPairs = new ArrayList<Pair<String,String>>();
    	if (BluetoothAdapter.getDefaultAdapter() == null) {
    		// simulate one of my devices
    		// F0-17-66-FC-5E-A1
    		if (hiveName.equals("Joe's Hive")) 
    			return "F0-17-66-FC-5E-A1";
    		else 
    			return null;
    	} else {
	    	int sz = NumHivesProperty.getNumHivesProperty(this);
	    	for (int i = 0; i < sz; i++) {
	    		if (PairedHiveProperty.getPairedHiveName(this, i).equals(hiveName)) 
	    			return PairedHiveProperty.getPairedHiveId(this, i);
	    	}
	    	return null;
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
	
	private PollSensorBackground.ResultCallback getOnCompletion(final int valueResid, final int timestampResid, final OnSaveValue saver) {
		PollSensorBackground.ResultCallback onCompletion = new PollSensorBackground.ResultCallback() {
			@Override
	    	public void report(String sensorType, final String timestampStr, final String valueStr) {
				runOnUiThread(new Runnable() {
					@Override
					public void run() {
						long timestamp = Long.parseLong(timestampStr)*1000;
						setValueWithSplash(valueResid, timestampResid, valueStr, true, timestamp);
						saver.save(MainActivity.this, valueStr, timestamp);
					}
				});
			}
			
			@Override
			public void error(String msg) {
				Log.e(TAG, "Error: "+msg);
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
						int cnt = 550; // start nearly full scale so that first poll is soon -- subsequent's will be spaced regularly
						while (!mStopPoller.get()) {
							try {
								Thread.sleep(95);
							} catch (InterruptedException e) {
								// TODO Auto-generated catch block
								e.printStackTrace();
							}
							
							// 95ms*600==57000ms
							if (cnt++ == 600) {
								cnt = 0;
								
								String ActiveHive = ActiveHiveProperty.getActiveHiveProperty(MainActivity.this);
								String HiveId = getHiveAddress(ActiveHive);
								boolean haveHiveId = HiveId != null;
								
								if (haveHiveId) {
									PollSensorBackground.ResultCallback onCompletion;
									HiveId = HiveId.replace(':', '-');
									
									onCompletion = getOnCompletion(R.id.cpuTempText, R.id.cpuTempTimestampText, new OnSaveValue() {
														@Override
														public void save(Activity activity, String value, long timestamp) {
															MCUTempProperty.setMCUTempProperty(MainActivity.this, value, timestamp);
														}
													});
						            new PollSensorBackground(DbHost, DbPort, Db, createQuery(HiveId, "cputemp"), onCompletion).execute();

									onCompletion = 
											getOnCompletion(R.id.tempText, R.id.tempTimestampText, new OnSaveValue() {
												@Override
												public void save(Activity activity, String value, long timestamp) {
											        TempProperty.setTempProperty(MainActivity.this, value, timestamp);
												}
											});
						            new PollSensorBackground(DbHost, DbPort, Db, createQuery(HiveId, "temp"), onCompletion).execute();
						            
									onCompletion = 
											getOnCompletion(R.id.humidText, R.id.humidTimestampText, new OnSaveValue() {
												@Override
												public void save(Activity activity, String value, long timestamp) {
											        HumidProperty.setHumidProperty(MainActivity.this, value, timestamp);
												}
											});
						            new PollSensorBackground(DbHost, DbPort, Db, createQuery(HiveId, "humid"), onCompletion).execute();
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
		
		super.onPause();
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		
		setInitialValues();
		startPolling();
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
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
		return super.onOptionsItemSelected(item);
	}
	
	public void onSettings() {
		Intent intent = new Intent(this, HiveSettingsActivity.class);
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

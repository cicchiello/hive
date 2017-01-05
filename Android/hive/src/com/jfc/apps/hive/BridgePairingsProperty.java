package com.jfc.apps.hive;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.util.Log;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.Toast;

import com.example.hive.R;
import com.jfc.misc.prop.IPropertyMgr;
import com.jfc.srvc.ble2cld.BluetoothPipeSrvc;
import com.jfc.util.misc.DialogUtils;
import com.jfc.util.misc.SplashyText;


public class BridgePairingsProperty implements IPropertyMgr {
	private static final String TAG = BridgePairingsProperty.class.getName();

	private static final int REQUEST_ENABLE_BT = 3210;
	
	private static final String HIVE_ID_PROPERTY = "HIVE_ID_PROPERTY";
	private static final String DEFAULT_HIVE_ID = "<unknown>";
	
    private static final int PERMISSION_REQUEST_COARSE_LOCATION = 1;
	private static final long SCAN_PERIOD = 12000;  // stop scanning after 12 seconds

    static final int grayColor = HiveEnv.ModifiableBackgroundColor;
	
    // created on constructions -- no need to save on pause
	private TextView mIdTv;
	private Activity mActivity;

	// transient variables -- no need to save on pause
	private AlertDialog mAlert;
	private Handler mScanCanceller = new Handler();
	private boolean mScanning;
	private Map<String,String> mReadableToAddresses = null;
    private ArrayAdapter<String> mDiscoveredDevices = null; 
	private Runnable mOnPermissionGranted = null;
	private List<Pair<String,String>> mExistingPairs = null;

	public static boolean isHiveIdPropertyDefined(Activity activity) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		return SP.contains(HIVE_ID_PROPERTY) && 
			!SP.getString(HIVE_ID_PROPERTY, DEFAULT_HIVE_ID).equals(DEFAULT_HIVE_ID);
	}
	
	public static String getHiveIdProperty(Activity activity) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		String id = SP.getString(HIVE_ID_PROPERTY, DEFAULT_HIVE_ID);
		return id;
	}
	
	public static void resetHiveIdProperty(Activity activity) {
		setHiveIdProperty(activity, DEFAULT_HIVE_ID);
	}
	
	public BridgePairingsProperty(final Activity activity, final TextView idTv, ImageButton button) {
		this.mActivity = activity;
		this.mIdTv = idTv;
		
    	mExistingPairs = new ArrayList<Pair<String,String>>();
    	if (BluetoothAdapter.getDefaultAdapter() == null) {
    		// simulate one of my devices
    		// F0-17-66-FC-5E-A1
	    	mExistingPairs.add(new Pair<String,String>("Joe's Hive", "F0-17-66-FC-5E-A1"));
    	} else {
	    	int sz = NumHivesProperty.getNumHivesProperty(mActivity);
	    	for (int i = 0; i < sz; i++) {
	    		String name = PairedHiveProperty.getPairedHiveName(mActivity, i);
	    		String id = PairedHiveProperty.getPairedHiveId(mActivity, i);
	    		mExistingPairs.add(new Pair<String,String>(name,id));
	    	}
    	}
    	
    	switch(mExistingPairs.size()) {
    	case 0: displayPairingState("no hives"); break;
    	case 1: displayPairingState("1 hive"); break;
    	default: displayPairingState(mExistingPairs.size()+" hives");
    	}
    	
    	if (BluetoothAdapter.getDefaultAdapter() != null) {
	    	button.setOnClickListener(new OnClickListener() {
				@Override
				public void onClick(View v) {
			    	
			        final ArrayAdapter<Pair<String,String>> arrayAdapter = new PairAdapter(mActivity, mExistingPairs);
			        
			        arrayAdapter.sort(new Comparator<Pair<String,String>>() {
						@Override
						public int compare(Pair<String,String> lhs, Pair<String,String> rhs) {
							return lhs.first.compareTo(rhs.first);
						}
					});
			        
			    	AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
			        builder.setIcon(R.drawable.ic_hive);
			        builder.setTitle(R.string.paired_hives);
			        builder.setPositiveButton(R.string.scan, new DialogInterface.OnClickListener() {
						@Override
						public void onClick(DialogInterface dialog, int which) {
							onScan();
						}
					});
			        builder.setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
			            @Override
			            public void onClick(DialogInterface dialog, int which) {
			            	mAlert.dismiss();
			            	mAlert = null;
			            }
			        });
			        builder.setNeutralButton(R.string.clear_all, new DialogInterface.OnClickListener() {
						@Override
						public void onClick(DialogInterface dialog, int which) {
			            	mAlert.dismiss();
			            	mAlert = null;
			        		Runnable okAction = new Runnable() {
			    				@Override
			    				public void run() {
					            	mAlert.dismiss();
					            	mAlert = null;
					            	NumHivesProperty.setNumHivesProperty(mActivity, 0);
					            	mExistingPairs = new ArrayList<Pair<String,String>>();
					            	displayPairingState("no hives");
					            	
		                			Intent ble2cldIntent= new Intent(mActivity, BluetoothPipeSrvc.class);
		                			ble2cldIntent.putExtra("cmd", "setup");
		                			mActivity.startService(ble2cldIntent);
			    				}
			    			};
			        		Runnable cancelAction = new Runnable() {
			    				@Override
			    				public void run() {
					            	mAlert.dismiss();
					            	mAlert = null;
			    				}
			    			};
			            	mAlert = DialogUtils.createAndShowAlertDialog(mActivity, R.string.delete_warning, 
			            			android.R.string.ok, okAction, android.R.string.cancel, cancelAction);
			            	
						}
					});
			        builder.setAdapter(arrayAdapter, null);
			        mAlert = builder.show();
				}
			});
    	} else {
	    	button.setOnClickListener(new OnClickListener() {
				@Override
				public void onClick(View v) {
		    		Toast.makeText(mActivity, "Bluetooth not supported on this device", Toast.LENGTH_LONG).show();
				}
			});
    	}
	}

	public AlertDialog getAlertDialog() {return mAlert;}

	private void setHiveIdUndefined() {
		mIdTv.setText(DEFAULT_HIVE_ID);
		mIdTv.setBackgroundColor(0xffff0000); // RED
	}
	
	private void displayPairingState(String msg) {
		mIdTv.setText(msg);
		mIdTv.setBackgroundColor(grayColor);
	}
	
	private void setHiveId(String msg) {
		setHiveIdProperty(mActivity, msg);
		displayPairingState(msg);
	}
	
	public static void setHiveIdProperty(Activity activity, String id) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		if (!SP.getString(HIVE_ID_PROPERTY, DEFAULT_HIVE_ID).equals(id)) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(HIVE_ID_PROPERTY, id);
			editor.commit();
		}
	}

	@Override
	public boolean onActivityResult(int requestCode, int resultCode, Intent intent) {
		if (resultCode == REQUEST_ENABLE_BT) {
    		selectFromBondedDevices();
			return true;
		}
		return false;
	}

	// Device scan callback.
	private BluetoothAdapter.LeScanCallback mLeScanCallback = new BluetoothAdapter.LeScanCallback() {
	    @Override
	    public void onLeScan(final BluetoothDevice device, int rssi,
	            byte[] scanRecord) {
	        mActivity.runOnUiThread(new Runnable() {
	           @Override
	           public void run() {
	        	   Log.i(TAG, device.getAddress());
	        	   if (!mReadableToAddresses.containsValue(device.getAddress())) {
	        		   for (Pair<String,String> p : mExistingPairs) {
	        			   if (p.second.equalsIgnoreCase(device.getAddress()))
	        				   return;
	        		   }
	        		   String readable = device.getName() + "   " + device.getAddress();
		               mReadableToAddresses.put(readable, device.getAddress());
	        		   mDiscoveredDevices.add(readable);
		               mDiscoveredDevices.notifyDataSetChanged();
	        	   } else {
	        		   Log.i(TAG, "Found it");
	        	   }
	           }
	       });
	   }
	};

	private void nameNewPairing(String defaultName, final String address) {
		final EditText input = new EditText(mActivity);
		
		String baseName = PairedHiveProperty.DEFAULT_PAIRED_HIVE_NAME;
		int baseNameSuffix = 0;
		boolean foundLegalName = false;
		while (!foundLegalName) {
			foundLegalName = true;
			for (Pair<String,String> p : mExistingPairs) {
				if (p.first.equals(baseName)) 
					foundLegalName = false;
			}
			if (!foundLegalName) {
				baseName = PairedHiveProperty.DEFAULT_PAIRED_HIVE_NAME + Integer.toString(++baseNameSuffix);
			}
		}
		input.setText(baseName);
		
		AlertDialog.Builder builder = 
				new AlertDialog.Builder(mActivity)
					.setIcon(R.drawable.ic_hive)
					.setTitle(R.string.choose_name)
					.setMessage(R.string.empty)
					.setView(input)
					.setPositiveButton("Ok", new DialogInterface.OnClickListener() {
	            		public void onClick(DialogInterface dialog, int whichButton) {
	            			String chosenName = input.getText().toString();
	            			boolean foundLegalName = true;
	            			for (Pair<String,String> p : mExistingPairs) {
	            				if (p.first.equals(chosenName)) 
	            					foundLegalName = false;
	            			}
	            			mAlert.dismiss();
	            			mAlert = null;
	            			
	            			if (foundLegalName) {
	            				mExistingPairs.add(new Pair<String,String>(chosenName, address));
	            				NumHivesProperty.setNumHivesProperty(mActivity, mExistingPairs.size());
	            				PairedHiveProperty.setPairedHiveId(mActivity, mExistingPairs.size()-1, address);
	            				PairedHiveProperty.setPairedHiveName(mActivity, mExistingPairs.size()-1, chosenName);
	            				
	            		    	switch(mExistingPairs.size()) {
	            		    	case 0: displayPairingState("no hives"); break;
	            		    	case 1: displayPairingState("1 hive"); break;
	            		    	default: displayPairingState(mExistingPairs.size()+" hives");
	            		    	}
	            		    	
	                    		SplashyText.highlightModifiedField(mActivity, mIdTv);
	                    		
	                			Intent ble2cldIntent= new Intent(mActivity, BluetoothPipeSrvc.class);
	                			ble2cldIntent.putExtra("cmd", "setup");
	                			mActivity.startService(ble2cldIntent);
	            			} else {
	        					Toast.makeText(mActivity, "Error: That name is already taken", Toast.LENGTH_LONG).show();
	            			}
	            		}
            		})
            		.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
	            		  public void onClick(DialogInterface dialog, int whichButton) {
	            		    // Canceled.
	            			  mAlert = null;
	            		  }
            		});
		mAlert = builder.show();
	}
	
    private void scanLeDevice(final boolean enable) {
        if (enable) {
            mDiscoveredDevices = new ArrayAdapter<String>(mActivity, R.layout.list_target);
            mReadableToAddresses = new HashMap<String,String>();
            
            AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
            builder.setIcon(R.drawable.ic_hive);
            builder.setTitle(R.string.choose_hive);
            builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            	@Override
            	public void onClick(DialogInterface dialog, int which) {
            		mAlert.dismiss(); 
            		scanLeDevice(false); // stop scanning
            		mAlert = null;
            	}
            });
            builder.setAdapter(mDiscoveredDevices, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                	String selection = mDiscoveredDevices.getItem(which);
            		mAlert.dismiss();
            		mAlert = null;
            		
            		scanLeDevice(false); // stop scanning
            		
            		String address = mReadableToAddresses.get(selection);
            		nameNewPairing(selection, address);
                }
            });
            mAlert = builder.show();

        	// Stops scanning after a pre-defined scan period.
            mScanCanceller.postDelayed(new Runnable() {
                @Override
                public void run() {
                    mScanning = false;
                    BluetoothAdapter.getDefaultAdapter().stopLeScan(mLeScanCallback);
                }
            }, SCAN_PERIOD);

            mScanning = true;
            BluetoothAdapter.getDefaultAdapter().startLeScan(mLeScanCallback);
        } else {
            mScanning = false;
            BluetoothAdapter.getDefaultAdapter().stopLeScan(mLeScanCallback);
        }
    }
    
	public void onScan() {
		final BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
		boolean isEnabled = adapter.isEnabled();
		if (!isEnabled) {
            final AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
            builder.setIcon(R.drawable.ic_hive);
            builder.setTitle("Bluetooth is off!");
            builder.setMessage(R.string.enable_bluetooth);
            builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
				@Override
				public void onClick(DialogInterface dialog, int which) {
					adapter.enable();
					requestLocationPermissionIfNeeded(new Runnable() {public void run() {scanLeDevice(true);}});
				}
			});
	        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
	        	@Override
	        	public void onClick(DialogInterface dialog, int which) {mAlert.dismiss(); mAlert = null;}
	        });
            mAlert = builder.show();
		} else {
			requestLocationPermissionIfNeeded(new Runnable() {public void run() {scanLeDevice(true);}});
		}
	}
	
	private void selectFromBondedDevices() {
        final ArrayAdapter<String> arrayAdapter = new ArrayAdapter<String>(mActivity, R.layout.list_target);
        final Map<String,BluetoothDevice> m = new HashMap<String,BluetoothDevice>();
        
    	// Loop through paired devices
        BluetoothAdapter mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        Set<BluetoothDevice> pairedDevices = mBluetoothAdapter.getBondedDevices();
    	for (BluetoothDevice device : pairedDevices) {
    		// Add the name and address to an array adapter to show in a ListView
    		String label = device.getName()+" : "+device.getAddress();
    		arrayAdapter.add(label);
    		m.put(label, device);
    	}

        AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
        builder.setIcon(R.drawable.ic_hive);
        builder.setTitle(R.string.choose_hive);
        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
        	@Override
        	public void onClick(DialogInterface dialog, int which) {mAlert.dismiss(); mAlert = null;}
        });
        builder.setAdapter(arrayAdapter, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
            	String selection = arrayAdapter.getItem(which);
        		mAlert.dismiss();
        		mAlert = null;
        		
        		nameNewPairing(selection, m.get(selection).getAddress());
            }
        });
        mAlert = builder.show();
	}

	@Override
	public void onPermissionResult(int requestCode, String[] permissions, int[] grantResults) {
        switch (requestCode) {
        case PERMISSION_REQUEST_COARSE_LOCATION: {
            // If request is cancelled, the result arrays are empty.
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            	Log.i(TAG, "Permissions granted");
            	mOnPermissionGranted.run();
            } else{
            	Log.e(TAG, "Permissions NOT granted");
            }
            return;
        }
        }
	}

	private void requestLocationPermissionIfNeeded(final Runnable onDismiss) {
	    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
	        // Android M Permission check
	        if (mActivity.checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
	            final AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
	            builder.setTitle("This app needs location access");
	            builder.setMessage("Please grant location access so this app can scan for Bluetooth peripherals");
	            builder.setPositiveButton(android.R.string.ok, null);
                mOnPermissionGranted = onDismiss;
	            builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
	                public void onDismiss(DialogInterface dialog) {
	                    mActivity.requestPermissions(new String[]{Manifest.permission.ACCESS_COARSE_LOCATION}, PERMISSION_REQUEST_COARSE_LOCATION);
	                }
	            });
	            mAlert = builder.show();
	        } else {
	        	onDismiss.run();
	        }
	    } else {
	    	onDismiss.run();
	    }
	}

	public class PairAdapter extends ArrayAdapter<Pair<String,String>> {
	    public PairAdapter(Context context, List<Pair<String,String>> users) {
	       super(context, 0, users);
	    }

	    @Override
	    public View getView(int position, View convertView, ViewGroup parent) {
	    	// Get the data item for this position
	    	Pair<String,String> pair = getItem(position); 
	    	
	    	// Check if an existing view is being reused, otherwise inflate the view
	    	if (convertView == null) {
	    		convertView = LayoutInflater.from(getContext()).inflate(R.layout.hive_pair, parent, false);
	    	}
	    	
	    	// Lookup view for data population
	    	TextView hiveName = (TextView) convertView.findViewById(R.id.hiveName);
	    	TextView hiveAddress = (TextView) convertView.findViewById(R.id.hiveAddress);
	    	
	    	// Populate the data into the template view using the data object
	    	hiveName.setText(pair.first);
	    	hiveAddress.setText(pair.second);
	    	
	    	// Return the completed view to render on screen
	    	return convertView;
	    }
	}
	
}

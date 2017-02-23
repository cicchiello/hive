package com.jfc.misc.prop;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
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

import com.jfc.apps.hive.HiveEnv;
import com.jfc.apps.hive.R;
import com.jfc.srvc.cloud.CouchGetBackground;
import com.jfc.util.misc.DialogUtils;
import com.jfc.util.misc.SplashyText;


public class BridgePairingsProperty implements IPropertyMgr {
	private static final String TAG = BridgePairingsProperty.class.getName();

	private static final String HIVE_ID_PROPERTY = "HIVE_ID_PROPERTY";
	private static final String DEFAULT_HIVE_ID = "<unknown>";
	
    private static final int PERMISSION_REQUEST_COARSE_LOCATION = 1;

    static final int grayColor = HiveEnv.ModifiableFieldBackgroundColor;
	
    // created on constructions -- no need to save on pause
	private TextView mIdTv;
	private Activity mActivity;

	// transient variables -- no need to save on pause
	private AlertDialog mAlert;
	private Set<String> mKnownHiveIds = null;
    private PairAdapter mDiscoveredDevices = null; 
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
	
	public static void resetHiveIdProperty(Context ctxt) {
		setHiveIdProperty(ctxt, DEFAULT_HIVE_ID);
	}
	
	public void onChange() {
		// do nothing, by default -- callers can extend to gain access
	}
	
	public BridgePairingsProperty(final Activity activity, final TextView idTv, ImageButton button) {
		this.mActivity = activity;
		this.mIdTv = idTv;
		
    	mExistingPairs = new ArrayList<Pair<String,String>>();
    	int sz = NumHivesProperty.getNumHivesProperty(mActivity);
    	for (int i = 0; i < sz; i++) {
    		String name = PairedHiveProperty.getPairedHiveName(mActivity, i);
    		String id = PairedHiveProperty.getPairedHiveId(mActivity, i);
    		mExistingPairs.add(new Pair<String,String>(name,id));
    	}
    	    	
    	switch(mExistingPairs.size()) {
    	case 0: displayPairingState("no hives"); break;
    	case 1: displayPairingState("1 hive"); break;
    	default: displayPairingState(mExistingPairs.size()+" hives");
    	}
    	
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
		        builder.setPositiveButton(R.string.find, new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						onFind();
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
				            	onChange();
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
	
	public static void setHiveIdProperty(Context ctxt, String id) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		if (!SP.getString(HIVE_ID_PROPERTY, DEFAULT_HIVE_ID).equals(id)) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(HIVE_ID_PROPERTY, id);
			editor.commit();
		}
	}

	@Override
	public boolean onActivityResult(int requestCode, int resultCode, Intent intent) {
		Toast.makeText(mActivity, "onActivityResult called unexpectedly", Toast.LENGTH_LONG).show();
		return false;
	}

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
	            				onChange();
	            				
	            		    	switch(mExistingPairs.size()) {
	            		    	case 0: displayPairingState("no hives"); break;
	            		    	case 1: 
	            		    		displayPairingState("1 hive"); 
	            		    		ActiveHiveProperty.setActiveHiveProperty(mActivity, chosenName);
	            		    		break;
	            		    	default: displayPairingState(mExistingPairs.size()+" hives");
	            		    	}
	            		    	
	                    		SplashyText.highlightModifiedField(mActivity, mIdTv);
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

	public static class PairAdapter extends ArrayAdapter<Pair<String,String>> {
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
	    	hiveAddress.setText(renderHiveId(pair.second));
	    	
	    	// Return the completed view to render on screen
	    	return convertView;
	    }
	    
		private String renderHiveId(String hiveId) {
			return "..."+hiveId.substring(20);
		}
		
	}
	
    private void onFind() {
		final String dbUrl = DbCredentialsProperty.getCouchConfigDbUrl(mActivity);
		final String authToken = null;
		
    	CouchGetBackground.OnCompletion onCompletion = new CouchGetBackground.OnCompletion() {
			@Override
			public void failed(final String msg) {
				mActivity.runOnUiThread(new Runnable() {
					@Override
					public void run() {
						Toast.makeText(mActivity, msg, Toast.LENGTH_LONG).show();
					}});
			}
			
			@Override
			public void complete(final JSONObject results) {
				try {
					JSONArray rows = results.getJSONArray("rows");
					for (int i = 0; i < rows.length(); i++) {
						JSONObject row = rows.getJSONObject(i);
						final String hiveId = row.getString("id");
						
						CouchGetBackground.OnCompletion hiveNameOnCompletion = new CouchGetBackground.OnCompletion() {
							@Override
							public void complete(final JSONObject hiveConfigDoc) {
								mActivity.runOnUiThread(new Runnable() {
									@Override
									public void run() {
										try {
											String hiveName = "<unnamed>";
											if (hiveConfigDoc.has("hive-name"))
												hiveName = hiveConfigDoc.getString("hive-name");
											
											mKnownHiveIds.add(hiveId);
											mDiscoveredDevices.add(new Pair<String,String>(hiveName,hiveId));
											mDiscoveredDevices.notifyDataSetChanged();
										} catch (JSONException je) {
											Log.e(TAG, "JSONException while trying to get config for: "+hiveId);
											mKnownHiveIds.add(hiveId);
											mDiscoveredDevices.add(new Pair<String,String>("<unknown>",hiveId));
											mDiscoveredDevices.notifyDataSetChanged();
										}
									}
								});
							}
							
							@Override
							public void failed(String msg) {
								mActivity.runOnUiThread(new Runnable() {
									@Override 
									public void run() {
										mKnownHiveIds.add(hiveId);
										mDiscoveredDevices.add(new Pair<String,String>("<unknown>",hiveId));
										mDiscoveredDevices.notifyDataSetChanged();
									}
								});
							}
						};
						new CouchGetBackground(dbUrl+"/"+hiveId, authToken, hiveNameOnCompletion).execute();
					}
				} catch (JSONException je) {
					failed(je.getMessage());
				}
			};
    	};
    	
    	final CouchGetBackground getter = new CouchGetBackground(dbUrl+"/_all_docs", authToken, onCompletion);
    	getter.execute();
    	
        mDiscoveredDevices = new PairAdapter(mActivity, new ArrayList<Pair<String,String>>());
        mKnownHiveIds = new HashSet<String>();
        
        AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
        builder.setIcon(R.drawable.ic_hive);
        builder.setTitle(R.string.choose_hive);
        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
        	@Override
        	public void onClick(DialogInterface dialog, int which) {
        		mAlert.dismiss(); 
        		getter.cancel(false);
        		mAlert = null;
        	}
        });
        builder.setAdapter(mDiscoveredDevices, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
            	Pair<String,String> selection = mDiscoveredDevices.getItem(which);
        		mAlert.dismiss();
        		mAlert = null;
        		
        		ActiveHiveProperty.setActiveHiveProperty(mActivity, selection.first);
        		
				NumHivesProperty.setNumHivesProperty(mActivity, 1);
				PairedHiveProperty.setPairedHiveId(mActivity, 0, selection.second);
				PairedHiveProperty.setPairedHiveName(mActivity, 0, selection.first);
				onChange();
            }
        });
        mAlert = builder.show();
    }
    
}

package com.jfc.apps.hive;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageButton;
import android.widget.TextView;

import com.example.hive.R;
import com.jfc.misc.prop.IPropertyMgr;
import com.jfc.util.misc.SplashyText;


public class ActiveHiveProperty implements IPropertyMgr {
	private static final String TAG = ActiveHiveProperty.class.getName();

	private static final String ACTIVE_HIVE_PROPERTY = "ACTIVE_HIVE_PROPERTY";
	private static final String DEFAULT_ACTIVE_HIVE = "???";
	
    static final int grayColor = HiveEnv.ModifiableBackgroundColor;
	
    // created on constructions -- no need to save on pause
	private TextView mActiveHiveTv;
	private Activity mActivity;

	// transient variables -- no need to save on pause
	private AlertDialog mAlert;
	private List<Pair<String,String>> mExistingPairs = null;

	
	public static boolean isActiveHivePropertyDefined(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		return SP.contains(ACTIVE_HIVE_PROPERTY) && 
			!SP.getString(ACTIVE_HIVE_PROPERTY, DEFAULT_ACTIVE_HIVE).equals(DEFAULT_ACTIVE_HIVE);
	}
	
	public static String getActiveHiveProperty(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		String id = SP.getString(ACTIVE_HIVE_PROPERTY, DEFAULT_ACTIVE_HIVE);
		return id;
	}
	
	public static void resetActiveHiveProperty(Activity activity) {
		setActiveHiveProperty(activity, DEFAULT_ACTIVE_HIVE);
	}
	
	private void loadExistingPairs(Activity activity)
	{
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
    	
    	if (mExistingPairs.size() == 1) {
    		setActiveHive(mExistingPairs.get(0).first);
    		setActiveHiveProperty(activity, mExistingPairs.get(0).first);
    	} else {
    		if (isActiveHivePropertyDefined(activity)) {
    			setActiveHive(getActiveHiveProperty(activity));
    		} else {
    			setActiveHiveUndefined();
    		}
    	}
	}

	public ActiveHiveProperty(final Activity activity, final TextView idTv, ImageButton button) {
		this.mActivity = activity;
		this.mActiveHiveTv = idTv;
		
		loadExistingPairs(activity);
		
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
    	
    	if (mExistingPairs.size() == 1) {
    		setActiveHive(mExistingPairs.get(0).first);
    		setActiveHiveProperty(activity, mExistingPairs.get(0).first);
    	} else {
    		if (isActiveHivePropertyDefined(activity)) {
    			setActiveHive(getActiveHiveProperty(activity));
    		} else {
    			setActiveHiveUndefined();
    		}
    	}
    	
    	button.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
		    	
				loadExistingPairs(activity);
				
		        final ArrayAdapter<Pair<String,String>> arrayAdapter = new PairAdapter(mActivity, mExistingPairs);
		        
		        arrayAdapter.sort(new Comparator<Pair<String,String>>() {
					@Override
					public int compare(Pair<String,String> lhs, Pair<String,String> rhs) {
						return lhs.first.compareTo(rhs.first);
					}
				});
		        
		    	AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
		        builder.setIcon(R.drawable.ic_hive);
		        builder.setTitle(R.string.active_hive_choice);
		        builder.setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
		            @Override
		            public void onClick(DialogInterface dialog, int which) {
		            	mAlert.dismiss();
		            	mAlert = null;
		            }
		        });
	            builder.setAdapter(arrayAdapter, new DialogInterface.OnClickListener() {
	                @Override
	                public void onClick(DialogInterface dialog, int which) {
	                	String selection = mExistingPairs.get(which).first;
	            		mAlert.dismiss();
	            		mAlert = null;
	            		
	            		setActiveHive(selection);
	            		setActiveHiveProperty(activity, selection);
                		SplashyText.highlightModifiedField(mActivity, mActiveHiveTv);
	                }
	            });
		        mAlert = builder.show();
			}
		});
	}

	public AlertDialog getAlertDialog() {return mAlert;}

	private void setActiveHiveUndefined() {
		mActiveHiveTv.setText(DEFAULT_ACTIVE_HIVE);
		mActiveHiveTv.setBackgroundColor(0xffff0000); // RED
	}
	
	private void displayPairingState(String msg) {
		mActiveHiveTv.setText(msg);
		mActiveHiveTv.setBackgroundColor(grayColor);
	}
	
	private void setActiveHive(String msg) {
		setActiveHiveProperty(mActivity, msg);
		displayPairingState(msg);
	}
	
	public static void setActiveHiveProperty(Activity activity, String id) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		if (!SP.getString(ACTIVE_HIVE_PROPERTY, DEFAULT_ACTIVE_HIVE).equals(id)) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(ACTIVE_HIVE_PROPERTY, id);
			editor.commit();
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

	@Override
	public boolean onActivityResult(int requestCode, int resultCode, Intent intent) {
		return false;
	}

	@Override
	public void onPermissionResult(int requestCode, String[] permissions, int[] grantResults) {
	}
	
}

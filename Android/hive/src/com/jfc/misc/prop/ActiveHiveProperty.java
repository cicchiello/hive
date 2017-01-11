package com.jfc.misc.prop;

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
import com.jfc.apps.hive.HiveEnv;
import com.jfc.util.misc.SplashyText;


public class ActiveHiveProperty implements IPropertyMgr {
	private static final String TAG = ActiveHiveProperty.class.getName();

	private static final String ACTIVE_HIVE_PROPERTY = "ACTIVE_HIVE_PROPERTY";
	private static final String DEFAULT_ACTIVE_HIVE = "???";
	
    static final int grayColor = HiveEnv.ModifiableFieldBackgroundColor;
	
    // created on constructions -- no need to save on pause
	private TextView mActiveHiveTv;
	private Context mCtxt;

	// transient variables -- no need to save on pause
	private AlertDialog mAlert;
	private List<Pair<String,String>> mExistingPairs = null;

	
	public static boolean isActiveHivePropertyDefined(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		return SP.contains(ACTIVE_HIVE_PROPERTY) && 
			!SP.getString(ACTIVE_HIVE_PROPERTY, DEFAULT_ACTIVE_HIVE).equals(DEFAULT_ACTIVE_HIVE);
	}
	
	public static String getActiveHiveProperty(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		String id = SP.getString(ACTIVE_HIVE_PROPERTY, DEFAULT_ACTIVE_HIVE);
		return id;
	}
	
	public static void resetActiveHiveProperty(Context ctxt) {
		setActiveHiveProperty(ctxt.getApplicationContext(), DEFAULT_ACTIVE_HIVE);
	}
	
	private void loadExistingPairs(Context ctxt)
	{
    	mExistingPairs = new ArrayList<Pair<String,String>>();
    	if (BluetoothAdapter.getDefaultAdapter() == null) {
    		// simulate one of my devices
    		// F0-17-66-FC-5E-A1
	    	mExistingPairs.add(new Pair<String,String>("Joe's Hive", "F0-17-66-FC-5E-A1"));
    	} else {
	    	int sz = NumHivesProperty.getNumHivesProperty(ctxt);
	    	for (int i = 0; i < sz; i++) {
	    		String name = PairedHiveProperty.getPairedHiveName(ctxt, i);
	    		String id = PairedHiveProperty.getPairedHiveId(ctxt, i);
	    		mExistingPairs.add(new Pair<String,String>(name,id));
	    	}
    	}
    	
    	if (mExistingPairs.size() == 1) {
    		setActiveHive(mExistingPairs.get(0).first);
    		setActiveHiveProperty(ctxt, mExistingPairs.get(0).first);
    	} else {
    		if (isActiveHivePropertyDefined(ctxt)) {
    			setActiveHive(getActiveHiveProperty(ctxt));
    		} else {
    			setActiveHiveUndefined();
    		}
    	}
	}

	public ActiveHiveProperty(final Activity activity, final TextView idTv, ImageButton button) {
		this.mCtxt = activity.getApplicationContext();
		this.mActiveHiveTv = idTv;
		
		loadExistingPairs(mCtxt);
		
    	mExistingPairs = new ArrayList<Pair<String,String>>();
    	if (BluetoothAdapter.getDefaultAdapter() == null) {
    		// simulate one of my devices
    		// F0-17-66-FC-5E-A1
	    	mExistingPairs.add(new Pair<String,String>("Joe's Hive", "F0-17-66-FC-5E-A1"));
    	} else {
	    	int sz = NumHivesProperty.getNumHivesProperty(mCtxt);
	    	for (int i = 0; i < sz; i++) {
	    		String name = PairedHiveProperty.getPairedHiveName(mCtxt, i);
	    		String id = PairedHiveProperty.getPairedHiveId(mCtxt, i);
	    		mExistingPairs.add(new Pair<String,String>(name,id));
	    	}
    	}
    	
    	if (mExistingPairs.size() == 1) {
    		setActiveHive(mExistingPairs.get(0).first);
    		setActiveHiveProperty(mCtxt, mExistingPairs.get(0).first);
    	} else {
    		if (isActiveHivePropertyDefined(mCtxt)) {
    			setActiveHive(getActiveHiveProperty(mCtxt));
    		} else {
    			setActiveHiveUndefined();
    		}
    	}
    	
    	button.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
		    	
				loadExistingPairs(mCtxt);
				
		        final ArrayAdapter<Pair<String,String>> arrayAdapter = new PairAdapter(mCtxt, mExistingPairs);
		        
		        arrayAdapter.sort(new Comparator<Pair<String,String>>() {
					@Override
					public int compare(Pair<String,String> lhs, Pair<String,String> rhs) {
						return lhs.first.compareTo(rhs.first);
					}
				});
		        
		    	AlertDialog.Builder builder = new AlertDialog.Builder(mCtxt);
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
	            		setActiveHiveProperty(mCtxt, selection);
                		SplashyText.highlightModifiedField(activity, mActiveHiveTv);
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
	
	public void setActiveHive(String msg) {
		setActiveHiveProperty(mCtxt, msg);
		displayPairingState(msg);
	}
	
	public static void setActiveHiveProperty(Context ctxt, String id) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
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

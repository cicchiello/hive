package com.jfc.util.misc;

import java.io.File;
import java.io.FileNotFoundException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Scanner;
import java.util.Set;

import android.content.Context;
import android.os.Build;
import android.os.Environment;
import android.util.Log;

/*
 * This class is mostly based upon code found at http://sapienmobile.com/?p=204
 * 
 * ...I just cleaned it up:
 *    1) make the class a proper Object instead of sort-of-kind-of all static
 *    2) removed the strange attempt to provide internationalized names for each mount point -- that's the responsibility of 
 *       the UI/View, not a utility function
 *    3) provide immutable accessors to provide enumeration-based classifications for the paths. 
 *    
 */
public class StorageOptions {
	private static final String TAG = StorageOptions.class.getSimpleName();
	
	public enum Type {
		internal, external
	};
	
	public StorageOptions(Context context) {
		Set<String> vold = new HashSet<String>();
		readVoldFile(vold);

		testAndCleanList(vold);

		List<Type> typeList = setProperties(context.getApplicationContext(), vold);
		
		paths = vold.toArray(new String[vold.size()]);
		types = typeList.toArray(new Type[typeList.size()]);
		if (types.length != paths.length){
			throw new IllegalStateException("device path and type lists aren't the same length!?!");
		}
	}

	public int numDevices() {return paths.length;}
	public Type getDeviceType(int index) {return types[index];}
	public String getDevicePath(int index) {return paths[index];}
	

	/*
	 * The rest of the class is private implementation
	 */
	
	private static final int HONEYCOMB = 11; // Build.VERSION_CODES.HONEYCOMB if sdk is high enough level

	private Type [] types;
	private String[] paths;

	private static void readVoldFile(Set<String> vold) {
		/*
		 * Scan the /system/etc/vold.fstab file and look for lines like this:
		 * dev_mount sdcard /mnt/sdcard 1
		 * /devices/platform/s3c-sdhci.0/mmc_host/mmc0
		 * 
		 * When one is found, split it into its elements and then pull out the
		 * path to the that mount point and add it to the set
		 * 
		 * some devices are missing the vold file entirely so we add a path here
		 * to make sure the list always includes the path to the first sdcard,
		 * whether real or emulated.
		 */

		vold.add("/mnt/sdcard");

		Scanner scanner = null;
		try {
			scanner = new Scanner(new File("/system/etc/vold.fstab"));
			while (scanner.hasNext()) {
				String line = scanner.nextLine();
				if (line.startsWith("dev_mount")) {
					String[] lineElements = line.split(" ");
					String element = lineElements[2];

					if (element.contains(":"))
						element = element.substring(0, element.indexOf(":"));

					if (element.contains("usb"))
						continue;

					// don't add the default vold path
					// it's already in the list.
					if (!vold.contains(element))
						vold.add(element);
				}
			}
		} catch (FileNotFoundException fnfe) {
			// don't care, and don't want to even dump anything to log
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			if (scanner != null) 
				scanner.close();
		}
	}

	private void testAndCleanList(Set<String> vold) {
		/*
		 * Now that we have a cleaned list of mount paths, test each one to make
		 * sure it's a valid and available path. If it is not, remove it from
		 * the list.
		 */

		Set<String> toRemove = new HashSet<String>();
		for (String voldPath : vold) {
			File path = new File(voldPath);
			if (!path.exists() || !path.isDirectory() || !path.canWrite())
				toRemove.add(voldPath);
		}
		vold.removeAll(toRemove);
	}

	private List<Type> setProperties(Context context, Set<String> vold) {
		/*
		 * At this point all the paths in the list should be valid. Build the
		 * public properties.
		 */

		List<Type> typeList = new ArrayList<Type>();

		if (vold.size() > 0) {
			if (Build.VERSION.SDK_INT < Build.VERSION_CODES.GINGERBREAD) {
				typeList.add(Type.internal);
			} else if (Build.VERSION.SDK_INT < HONEYCOMB) {
				typeList.add(Environment.isExternalStorageRemovable() ? Type.external : Type.internal);
			} else {
				boolean isInternal = false;
				if (!Environment.isExternalStorageRemovable()) {
					isInternal = true;
				} else {
					// may be able to ask Environment.isExternalStorageEmulated, depending on api level
					Method m = null; 
					try {
						m = Environment.class.getMethod("isExternalStorageEmulated", new Class[] {} );
					} catch (NoSuchMethodException nsme) {/*nothing to do -- leave m==null*/}
					if (m != null) {
						Object r = null;
						try {
							r = m.invoke(null, new Object[] {});
						} catch (IllegalArgumentException e) {
							Log.e(TAG, "IllegalArgumentException received while trying to invoke Environment.isExternalStorageEmulated!?!?");
						} catch (IllegalAccessException e) {
							Log.e(TAG, "IllegalAccessException received while trying to invoke Environment.isExternalStorageEmulated!?!?");
						} catch (InvocationTargetException e) {
							Log.e(TAG, "InvocationTargetException received while trying to invoke Environment.isExternalStorageEmulated!?!?");
						}
						if ((r != null) && (r instanceof Boolean) && ((Boolean) r).booleanValue()) 
							isInternal = true;
					}
				}
				
				typeList.add(isInternal ? Type.internal : Type.external);
			}

			for (int i = 1; i < vold.size(); i++) {
				typeList.add(Type.external);
			}
		}

		return typeList;
	}
}

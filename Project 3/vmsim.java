import java.io.*;
import java.util.*;
import java.lang.*;

public class vmsim {
	
	// static ArrayList<String> listOfTraceFileLines = new ArrayList<String>();
	static Hashtable<String, ArrayList<Integer>> addressHT = new Hashtable<String, ArrayList<Integer>>();

	public static void main(String [] args) {
		int numOfFrames = 0, refresh = 0;
		String algo = "";
		File traceFile = null;
		PageTable pt = null;

		//Check if there are any command line arguments
		if (args.length > 0) {
			//Set the number of frames
			numOfFrames = Integer.parseInt(args[1]);
			// System.out.println("Frames: " + numOfFrames);

			//Get the algorithm that is to be used
			algo = args[3];
			// System.out.println("Algorithm: " + algo);

			//If the algorithm is NRU, then we need to get the refresh value
			//Then get the trace file
			if (algo.equals("nru") || algo.equals("aging")) {
				refresh = Integer.parseInt(args[5]);
				// System.out.println("Refresh: " + refresh);
				traceFile = new File(args[6]);
			}
			else
				traceFile = new File(args[4]);
			// System.out.println("TraceFile: " + traceFile);
		}
		else
			System.exit(0);

		if (algo.equals("opt")) {
			pt = new Opt(numOfFrames);

		}

		if (algo.equals("clock")) {
			pt = new Clock(numOfFrames);
		}

		if (algo.equals("nru")) {
			pt = new NRU(numOfFrames);
		}

		if (algo.equals("aging")) {
			pt = new Aging(numOfFrames);
			
		}

		//Varibale to keep track of the total number of lines read from the .trace file
		//Also represents the total number of memory accesses
		int lineCounter = 0;
		try {
			//Now I need to read the file
			Scanner fileScan = new Scanner(traceFile);
			String nextLine[], address;
			//op is true if it is a write and false if it is a read
			boolean op = false;

			//This set will hold the set of keys (addresses) in the Hashtable
			Set<String>htKeys = null;

			//If the algorithm is opt, read the file into a Hashtable using the optRead method
			//Then copy that Hashtable over to the Hashtable in PageTable.java
			if (algo.equals("opt")) {
				optRead(traceFile);

				pt.setHashtable(addressHT);
				htKeys = addressHT.keySet();
				// System.out.println("Key Set: " + htKeys.toString());
			}

			//Else we can just read the file line by line
			do {
				//Split the line from the .trace file into a String array
				nextLine = fileScan.nextLine().split(" ");
				//Get the first 4 characters of the memory address
				//I do this because the rest of the address is just an offset to some instructions (I think)
				address = nextLine[0].substring(0, 5);
				//If it is a Write operation, set op to true
				if (nextLine[1].equals("W"))
					op = true;
				//If it is a read operation, set to false
				else if (nextLine[1].equals("R"))
					op = false;

				//Call the getPage method in the PageTable class on this new Page
				pt.getPage(address, op);

				// System.out.println("Address: " + address + "\nOperation: " + nextLine[1] + " " + op);

				//If the algorithm is NRU or Aging and we are not reading the first line,
				//and we have read the number of lines for a refresh
				if (refresh != 0 && lineCounter != 0 && lineCounter % refresh == 0) {
					if (algo.equals("nru"))
						pt.refresh();
					else if (algo.equals("aging"))
						pt.agingRefresh();
				}
					

				//If we are running opt, use a for loop to iterate through the 
				if (algo.equals("opt") && htKeys != null) {
					Iterator<String> keyIter = addressHT.keySet().iterator();
					
					//Iterate through the set of Hashtable keys in order to find
					//if the value at one of the keys is less than or equal to the current line
					for (String key : htKeys) {
						//If the ArrayList at the address is not empty
						if (!addressHT.get(key).isEmpty()) {
							if (addressHT.get(key).get(0) <= lineCounter)
								addressHT.get(key).remove(0);
						}
					}

					//Iterate again through the the Hashtable keys and check if any of their
					//Values (ArrayList) are empty. If so then we remove the ArrayList and make the
					//Value = null
					while (keyIter.hasNext()) {
						String key = keyIter.next();
						if (addressHT.get(key).isEmpty())
							keyIter.remove();
					}
					//Copy over the new Hashtable to the PageTable class
					pt.setHashtable(addressHT);
					// System.out.println("Line: " + lineCounter);
					// System.out.println("Hashtable: " + addressHT.toString());

				}

				lineCounter++;
			}while (fileScan.hasNextLine() && lineCounter < 1000000);
			

		}
		catch (FileNotFoundException e) {
			System.out.println(e);
			System.exit(0);
		}
		catch (IOException e1) {
			System.out.println(e1);
			System.exit(0);
		}

		System.out.println(algo);
		System.out.println("Number of frames: \t" + numOfFrames);
		System.out.println("Total memory accesses:  " + lineCounter);
		System.out.println("Total page faults: \t" + pt.getPageFaults());
		System.out.println("Total writes to disk: \t" + pt.getWrites());
	}

	//Reads the file line by line and puts only the first four chars of the address and the R or W operation into
	//the ArrayList

	//This method also populates a Hashtable with a Key that is an address and a Value that is
	//an ArrayList containing the line numbers in which the address occurs in the .trace file
	public static void optRead(File file) throws IOException {
		Scanner fScan = new Scanner(file);
		int lineCounter = 0;
		ArrayList<Integer> addressOccurrences;
		while (fScan.hasNextLine() && lineCounter < 1000000) {
			String currentLine = fScan.nextLine();

			// listOfTraceFileLines.add(currentLine.substring(0, 4) + " " + currentLine.substring(9, 10));

			//If the Hashtable does not contain the current address, then add it to the Hashtable
			//with the address as the Key and an ArrayList as the Value
			if (!addressHT.containsKey(currentLine.substring(0, 5))) {
				addressOccurrences = new ArrayList<Integer>();
				//Add the line number of where this address occurs in the file to the ArrayList
				addressOccurrences.add(lineCounter);

				//Put this new Key (address) and Value (ArrayList) into the Hashtable
				addressHT.put(currentLine.substring(0, 5), addressOccurrences);
				
			}
			//Else put the line number of when the address is referenced in the .trace file
			//get() gets the value at that key in the Hashtable, in other words it returns the ArrayList for that Key (address)
			else
				addressHT.get(currentLine.substring(0, 5)).add(lineCounter);
			lineCounter += 1;
		}

		// System.out.println("Hashtable: \n" + addressHT.size());
	}

	
}
import java.io.File;
import java.util.*;

public abstract class PageTable {
	int numOfPageFaults = 0, numOfWrites = 0;
	Page[] table;
	Hashtable<String, ArrayList> optHashtable = new Hashtable<String, ArrayList>();
	//Initalize a Pagetable to be the size of the number of frames, which is taken in from the command line
	public PageTable(int frames) {
		table = new Page[frames];
		for (int i = 0; i < frames; i++)
			table[i] = new Page("-1", false, false);	//initialize the frame number of the pages to -1 and they're bits to false
	}

	//getPage will check if the page is in the page table
	//address param is the address that was read from the file
	//op param is either W or R. W = true, R = false
	public void getPage(String address, boolean op) {

		// for (int i = 0; i < table.length; i++)
		// 	System.out.println(table[i].getAddress() + " " + Arrays.toString(table[i].getAgingCounter()));
		// System.out.println();

		Page pageToAdd = new Page(address, op, true);
		//initialize the 8 bit counter for the Aging algorithm
		pageToAdd.initializeAgingValue();

		//Search through to find the Page
		for (int i = 0; i < table.length; i++) {
			//If the page is found
			if (table[i].getAddress().equals(address)) {
				// System.out.println("hit");

				//Update the reference bit to 1
				table[i].setRefBit(true);
				//If the Dirty Bit is 1, then update the Page's Dirty Bit to 1
				if (pageToAdd.getDirtyBit())
					table[i].setDirtyBit(true);

				//Set the temporary reference bit to 1 for the Aging algorithm
				pageToAdd.setTempRefBit(true);

				return;
			}

			//If the index is empty, insert the Page into that index
			if (table[i].getAddress().equals("-1")) {
				// System.out.println("page fault - no eviction");
				table[i] = pageToAdd;
				numOfPageFaults++;

				//Set the temporary reference bit to 1 for the Aging algorithm
				pageToAdd.setTempRefBit(true);

				return;
			}
		}

		// for (int j = 0; j < table.length; j++)
		// 	System.out.println(table[j].getAddress() + " R:" + table[j].getRefBit() + " D:" + table[j].getDirtyBit());

		//Page was not found
		//Use the algorithm specific eviction
		Page pageToEvict = pageEvictionSelection();
		// System.out.println("Evicting Page:");
		// System.out.println("Address: " + pageToEvict.getAddress() + "\nDirty Bit: " + pageToEvict.getDirtyBit() 
			// + "\nRef Bit: " + pageToEvict.getRefBit());

		evictPage(pageToEvict, pageToAdd);

		//Set the temporary reference bit to 1 for the Aging algorithm
		pageToAdd.setTempRefBit(true);
	}

	public void evictPage(Page evict, Page add) {
		for (int i = 0; i < table.length; i++) {
			//Find the page that we want to evict in our array
			if (table[i].equals(evict)) {

				// System.out.println("Add\nAddress: " + add.getAddress() + " Dirty: " + add.getDirtyBit());
				// System.out.println("Evict\nAddress: " + evict.getAddress() + " Dirty: " + evict.getDirtyBit());

				//If the Dirty Bit of the Page we want to evict is 1, then we increment the number of writes to disk
				if (table[i].getDirtyBit()) {
					numOfWrites++;
					// System.out.println("page fault - evict dirty");
				}	
				// else
				// 	System.out.println("page fault - evict clean");

				//Replace the Page at that index with the new Page	
				table[i] = add;
				numOfPageFaults++;

				// System.out.println("Clock - after");
				// for (int j = 0; j < table.length; j++)
				// 	System.out.println(table[j].getAddress() + " R:" + table[j].getRefBit() + " D:" + table[j].getDirtyBit());

				return;
			}
		}
	}

	//Abstract method that will select which page will be evicted
	//This will be different for every algorithm
	abstract Page pageEvictionSelection();
	
	public int getPageFaults() {
		return numOfPageFaults;
	}

	public int getWrites() {
		return numOfWrites;
	}

	//Method for NRU and Aging that will reset all the reference bits in the array
	public void refresh() {
		//Refresh will go through the Pages and set each of their reference bits to 0
		for (int i = 0; i < table.length; i++)
			table[i].setRefBit(false);
	}

	public void agingRefresh() {
		for (int i = 0; i < table.length; i++) {
			table[i].setAgingValue(table[i].getTempRefBit());
			table[i].setTempRefBit(false);
		}
			
	}

	//Method for Optimal algorithm which will copy the Hashtable passed in to the global one declared
	//in PageTable.java
	public void setHashtable(Hashtable ht) {
		optHashtable = ht;
	}

	//Class for Pages that are stored within the array
	class Page {
		//frameindex will keep track of the address of the Page
		String address;
		//Two booleans for the dirty and reference bit
		boolean dirtyBit, referenceBit;
		int [] agingValue = new int [8];
		boolean tempRefBit;

		//Constructor
		public Page(String fn, boolean dirty, boolean ref) {
			this.address = fn;
			this.dirtyBit = dirty;
			this.referenceBit = ref;

		}

		public String getAddress() {
			return address;
		}

		public boolean getRefBit() {
			return referenceBit;
		}

		public boolean getDirtyBit() {
			return dirtyBit;
		}

		public void setRefBit(boolean ref) {
			this.referenceBit = ref;
		}

		public void setDirtyBit(boolean dirty) {
			this.dirtyBit = dirty;
		}

		public void setTempRefBit(boolean ref) {
			this.tempRefBit = ref;
		}

		public boolean getTempRefBit() {
			return tempRefBit;
		}

		public void initializeAgingValue() {
			for (int i = 0; i < agingValue.length; i++)
				agingValue[i] = 0;
		}

		public void setAgingValue(boolean ref) {
			//If the Page is referenced shift a 1 into the aging value (8 bits)
			if (ref) {
				System.arraycopy(agingValue, 0, agingValue, 1, agingValue.length - 1);
				agingValue[0] = 1;
			}
				
			//If shift a 0 into the aging value (8 bits)
			else {
				System.arraycopy(agingValue, 0, agingValue, 1, agingValue.length - 1);
				agingValue[0] = 0;
			}
				
		}

		public int[] getAgingCounter() {
			return agingValue;
		}

	}
}
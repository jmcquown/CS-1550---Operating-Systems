public class Opt extends PageTable {
	public Opt(int frames) {
		super(frames);
	}

	//For the optimal algorithm I need to check each address in the array and evict
	//the address which is called on the line number the furthest away
	public Page pageEvictionSelection() {
		String pageAddress = super.table[0].getAddress();

		//Initialize our highestOccurrence int to 0
		//highestOccurrence represents the highest line number that a Page is referenced in the .trace file
		int highestOccurrence = 0;

		
		//If the Value (ArrayList) at the first Page in the array in the Hashtable is not null
		//set it equal to the current highest occurrence
		// if (super.optHashtable.get(pageAddress) != null)
		// 	highestOccurrence = (int) super.optHashtable.get(pageAddress).get(0);

		//indexOfPage keeps track of the index of the Page with the highest occurrence
		int indexOfPage = 0;

		//Iterate through the array of Pages
		for (int i = 0; i < super.table.length; i++) {
			// System.out.println("Current Page: " + super.optHashtable.get(super.table[i].getAddress()));

			//If the Value (ArrayList) at the Key (address of Page) is not null
			if (super.optHashtable.get(super.table[i].getAddress()) != null) {
				//If the current page in the table has an occurrence that is higher than the current highest occurrence
				if ((int) super.optHashtable.get(super.table[i].getAddress()).get(0) > highestOccurrence) {
					highestOccurrence = (int) super.optHashtable.get(super.table[i].getAddress()).get(0);
					indexOfPage = i;
				}
			}
			//If the value of the Page's ArrayList is null, that means that the Page will not referenced
			//ever again in the .trace file
			else {
				indexOfPage = i;
				break;
			}
				
		}

		// for (int i = 0; i < super.table.length; i++)
		// 	System.out.println(super.table[i].getAddress());

		// System.out.println("Address: " + super.table[indexOfPage].getAddress());
		// System.out.println("Highest Occurrence: " + highestOccurrence);
		// System.out.println("ArrayList: " + super.optHashtable.get(super.table[indexOfPage].getAddress()));

		// System.out.println(super.optHashtable.get(super.table[indexOfPage].getAddress()));

		//Return the Page that we want to evict
		return super.table[indexOfPage];
	}
}
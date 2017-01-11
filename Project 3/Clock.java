public class Clock extends PageTable {
	int clockHand = 0;
	public Clock(int frames) {
		super(frames);
	}

	//Method that will select what Page to evict in the PageTable based on the Clock Algorithm
	public Page pageEvictionSelection() {
		boolean found = false;
		//clockHand represents the hand of the clock, or the element that is being pointed to in the circular queue
		
		while (!found) {

			//Print statements for testing
			// System.out.println("Address: " + super.table[counter % super.table.length].getAddress());
			// System.out.println("Ref Bit: " + super.table[counter % super.table.length].getRefBit());

			//If the current Page in the PageTable has a reference bit of 0 set found to true to stop looping
			if (!super.table[clockHand % super.table.length].getRefBit())
				break;
			//Else set the reference bit to 0 and move on to the next Page in the PageTable
			
			else
				super.table[clockHand % super.table.length].setRefBit(false);
			clockHand += 1;
		}	

		//Return the Page
		Page evict = super.table[clockHand % super.table.length];
		clockHand++;
		return evict;
	}

}
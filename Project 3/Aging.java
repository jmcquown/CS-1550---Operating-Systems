import java.util.Arrays;
public class Aging extends PageTable {
	public Aging(int frames) {
		super(frames);
	}

	//Aging algorithm will choose the Page will the lowest value for its 8 bit number
	public Page pageEvictionSelection() {
		


		//lowestCounter will keep track of value of lowest counter (8 bit) in our array of Pages
		int lowestCounter = 0;
		//pageIndex will keep track of the index of the Page in the array with the lowest counter
		int pageIndex = 0;


		int [] agingCounter = super.table[0].getAgingCounter();
		// System.out.println("Initial counter: " + Arrays.toString(agingCounter));
		//For loop iterates through the 8 bit counter array and calculates the sum
		for (int j = 0; j < agingCounter.length; j++) {
			//If the ref bit at the index j is 1
			if (agingCounter[j] == 1)
				lowestCounter += (int) Math.pow(2, 7-j);
		}

		// System.out.println("Initial lowest: " + lowestCounter);
		// System.out.println("Initial Address: " + super.table[0].getAddress());

		for (int i = 1; i < super.table.length; i++) {
			agingCounter = super.table[i].getAgingCounter();
			int agingValue = 0;
			for (int j = 0; j < agingCounter.length; j++) {
				if (agingCounter[j] == 1)
					agingValue += (int) Math.pow(2, 7-j);
			}
			// System.out.println("Current counter value: " + agingValue);
			// System.out.println("Current Address: " + super.table[i].getAddress());
			// System.out.println(Arrays.toString(agingCounter));
			if (agingValue < lowestCounter) {
				lowestCounter = agingValue;
				pageIndex = i;
			}
			// System.out.println("New counter value: " + lowestCounter);
		}

		return super.table[pageIndex];
	}
}
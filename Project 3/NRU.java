public class NRU extends PageTable {
	public NRU(int frames) {
		super(frames);
	}

	//Iterate through the Pages
	//Hierarchy of Pages we want to evit is:
	//Ref:0 Dirty:0 | Ref:0 Dirty:1 | Ref:1 Dirty:0 | Ref:1 Dirty: 1
	public Page pageEvictionSelection() {
		Page pageToEvict = super.table[0];
		for (int i = 0; i < super.table.length; i++) {
			if (!super.table[i].getRefBit() && !super.table[i].getDirtyBit()) {
				pageToEvict = super.table[i];
				break;
			}
			
			//If the Page we want to evict's ref bit is 1 and the current Page we are at in the array
			//has a ref bit of 0, then set the Page we want to evict equal to the one we are currently looking at
			//in the array
			if (pageToEvict.getRefBit() && !super.table[i].getRefBit()) {
				pageToEvict = super.table[i];
				continue;
			}
			//If the current Page we want to evict and the Page we are at in the array have to same value for
			//their ref bit check the dirty bit
			if (pageToEvict.getRefBit() == super.table[i].getRefBit()) {
				//If pageToEvict's dirty bit is 1 and table[i]'s dirty bit is 0
				if (pageToEvict.getDirtyBit() && !super.table[i].getDirtyBit())
					pageToEvict = super.table[i];
				//Else their dirty bits are the same, or table[i]'s dirty bit is 1 when pageToEvict's is 0
				continue;
			}	
				
		}

		return pageToEvict;
	}
}
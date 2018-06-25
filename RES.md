# run
* performance
          
	$ make rand
* recovery

	$ make recovery
* unit tests

	$ make run_test_suite

# performance
* pure original: 2097727
* gf: 2078126
* gf+flush: 2081245
* mod: 2094927
* mod+flush: 2078118
* size: 2074616
* size+flush: 2055173
* clone: 2091452
* clone+flush: 2070761
* loggedepoch: 2137146
* loggedepoch+flush: 2118240
* init loggedepoch+flush: 2115338
* log logic: 2119952
* log logic+flush: 2095439
* recovery: seems to work with mt

## using flush
* rand without logging: 2098709





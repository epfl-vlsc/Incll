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

## using global flush default
* rand without logging: 2098709
* rand with logging: 1804248
* rand with logging+flush(node): 1719341
* rand with logging+flush(node)+flush(tail): 1603517

## key change to 12 + gf
* rand without logging: 1973669
* rand with logging+flush(node)+flush(tail): 1559236

## assumptions rand
* disabled need_phantom_epoch in masstree.h
* deallocs possible to disable: for measurements enabled

* rand with logging+flush(node)+flush(tail): 1554701
* logging distributed from find_locked(config above): 1652645

possible explanation: if not found don't log for remove

##incll(with above assumptions)
* moved logging to incll locations: 1636703
* logging+flush(node)+flush(tail)+incll(fields): 1618616
* logging+flush(node)+flush(tail)+incll(fieldsv2): 1630636
* logging+flush(node)+flush(tail)+incll(fieldsv3): 1629900
* global log root keeping + above: 1638092

## incll recovery works
* lg+flush(node,tail)+incll(insert,update)+no recover immediate: 1845163
* lg+flush(node,tail)+incll(insert,update)+lazy recovery: 1835070

## all above with 14 keys
* lg+flush(node,tail)+incll(insert,update)+lazy recovery: 1923693

## optimization trials:lg+flush(node,tail)+incll(insert,update)+lazy recovery
* layout opt1: 1937639
* extlog opt1: 1940516
* inline opt1: 1942045







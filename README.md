#Homework for "Operating system" class

There is a server that handles client orders in separate processes. 
Client data are storing in shared memory, access to which is managed by semaphores.
Clients can book seats, get current schema of the plane or list of passengers.

```
$ ./cli 127.0.0.1
[20:48:6:437220 INFO] (client.c:25) connecting to 127.0.0.1:3490
[20:48:6:437244 INFO] (client.c:30) enter message:
> get_schema
[20:48:9:760961 INFO] (client.c:48) server reply:
                      +--+
                      | * \
                      | *  \
                  +---+ *   \
                  +---+ *    \
+---+                 | *     \
| *  \  +---------------------------------------+
+ *   \ | 01 |    02 |    03 |    04 |    05 | | \
\  *   \| 06 |    07 |    08 |    09 |    10 | |  \
 \  *   | 11 |    12 |    13 |    14 |    15 | |   \
  +-----+                                      |  --\
  +-----+                                      |  --/
 /  *   | 16 |    17 |    18 |    19 |    20 | |   /
/  *   /| 21 |    22 |    23 |    24 |    25 | |  /
+ *   / | 26 |    27 |    28 |    29 |    30 | | /
| *  /  +---------------------------------------+
+---+                 | *     /
                  +---+ *    /
                  +---+ *   /
                      | *  /
                      | * /
                      +--+
	Total seats: 30
	Smoking seats: 1 6 11 16 21 26
	Available tickets: 30
[20:48:9:760997 INFO] (client.c:30) enter message:
> buy_ticket 22 Barack Obama
[20:49:16:335613 INFO] (client.c:48) server reply:
OK
```

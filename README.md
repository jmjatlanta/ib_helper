Some helpful front-ends to IB TWS

To Test: 
- A cancel order comes in, the order is executed, the cancel is rejected because the order is filled
   - Need the ability to delay execution of an order
   - Need to handle cancel errors exactly as IB does it (what error code is thrown in this case?)
- A cancel order comes in, the order is partially executed, the cancel succeeds but the position is probably not what you expect
   - Need the ability to delay and partially execute an order
- A partial fill happens, then a TP. The original order is still there. Should it be?
NOTE: what happens in these cases are strategy dependent, but we need the ability to test this)


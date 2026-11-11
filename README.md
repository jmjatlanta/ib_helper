## Some helpful code to interface with IB TWS in C++

I often build applications that interface with Interactive Brokers. This repository helps by
- Breaking down the `EWrapper.hpp` into sections so I have to implement less
- Provide some mock classes to test things without having TWS running, and using saved tick or bar data

This project uses CMake. Some notes:
- The Decimal Floating Point library from Intel is used by IB TWS. What comes with their installer is not always compatible. To solve this, I have added Intel's library as a subproject and compile it as part of this build
- The date project from HowardHinnant is used to help with contract rollover date stuff.
- The directory that contains the IB TWS API must be provided.

```
git clone https://github.com/jmjatlanta/ib_helper
cd ib_helper
git submodule update --init --recursive
mkdir build
cd build
cmake .. -DIB_CLIENT_SOURCE_DIR=/home/mydir/IBJts/source
make
```
On macOS, also pass `-DHH_DATELIB=On` (Apple's libc++ has no `std::chrono` time zones).

### Using ib_helper as a subproject
ib_helper can be added to another CMake project with `add_subdirectory()`. Its unit tests (which download
googletest, and some of which need a running TWS) are only built by default when ib_helper is the top-level
project; set `IB_HELPER_BUILD_TESTS` to override.
```cmake
set(IB_CLIENT_SOURCE_DIR "$ENV{HOME}/IBJts/source" CACHE PATH "TWS API source directory")
add_subdirectory(external/ib_helper)
target_link_libraries(my_app PRIVATE ib_helper::ib_helper)  # also brings in the TWS API (ib_helper::ib_api)
```

Things left TODO (Mock/testing):
- A cancel order comes in, the order is executed, the cancel is rejected because the order is filled
   - Need the ability to delay execution of an order
   - Need to handle cancel errors exactly as IB does it (what error code is thrown in this case?)
- A cancel order comes in, the order is partially executed, the cancel succeeds but the position is probably not what you expect
   - Need the ability to delay and partially execute an order
- A partial fill happens, then a TP. The original order is still there. Should it be?
NOTE: what happens in these cases are strategy dependent, but we need the ability to test this)

# Jet ESP-IDF shim

The upstream Jet component from the IDF Component Registry / GitHub does not
include a root `CMakeLists.txt`. After `idf.py reconfigure`, ensure this file
exists:

```
managed_components/jet/CMakeLists.txt
```

Copy from `basic_engine_test_game/managed_components/jet/CMakeLists.txt` if
missing. Also edit `managed_components/jet/src/JetConfig.hpp` for display
settings (240×240 round display).

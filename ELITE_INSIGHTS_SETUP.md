# GW2 RotaHelper - Integration with Elite Insights Parser

This project integrates with the GW2 Elite Insights Parser to parse ArcDPS log files.

## Setup Instructions

### 1. Download Elite Insights Parser CLI

1. Go to the [GW2 Elite Insights Parser releases page](https://github.com/baaron4/GW2-Elite-Insights-Parser/releases/latest)
2. Download the `GW2EI.zip` file
3. Extract the contents to your project directory or a location in your PATH
4. Make sure `GuildWars2EliteInsights-CLI.exe` is accessible

### 2. Configuration

The project includes an `ei_config.conf` file that configures the parser to:
- Output JSON format
- Enable parsing phases and damage modifiers
- Save outputs in the same directory as input files
- Skip failed parsing attempts

### 3. Usage

1. Run your GW2RotaHelper application
2. Click "Select File" to choose an ArcDPS log file (.evtc or .zevtc)
3. Click "Parse Log" to process the file
4. The parser will generate a JSON file with the same name as your log file

### 4. Supported File Formats

- `.evtc` - Raw ArcDPS log files
- `.zevtc` - Compressed ArcDPS log files  
- `.evtc.zip` - Zipped ArcDPS log files

### 5. JSON Output Structure

The Elite Insights Parser generates comprehensive JSON output containing:
- Player information and stats
- Skill usage and damage data
- Buff/debuff information
- Phase data
- Combat mechanics
- And much more...

You can use the nlohmann/json library (already included) to parse this data in your C++ application.

### Example Integration

```cpp
#include <nlohmann/json.hpp>
#include <fstream>

// After successful parsing, read the JSON file
std::ifstream jsonFile(jsonPath);
nlohmann::json logData;
jsonFile >> logData;

// Access parsed data
auto players = logData["players"];
auto targets = logData["targets"];
// ... process the data as needed
```

## Alternative Options

If you prefer not to use the CLI approach, you could also consider:

1. **C++/CLI Wrapper**: Create a C++/CLI wrapper around the C# library
2. **COM Interop**: Use COM to call the C# library from C++
3. **Process Communication**: Use named pipes or other IPC mechanisms
4. **Direct Binary Parsing**: Implement your own EVTC parser (complex but possible)

## More Information

- [Elite Insights Parser Documentation](https://github.com/baaron4/GW2-Elite-Insights-Parser)
- [JSON Output Documentation](https://baaron4.github.io/GW2-Elite-Insights-Parser/Json/index.html)
- [ArcDPS Documentation](https://www.deltaconnected.com/arcdps/)


#include "MxOptim.h"
#include "mGlobalSignals.h"

EmbeddedPython::EmbeddedPython(const std::string& runtime_dir) {
#ifdef _WIN32
        char abs_path[MAX_PATH];
        GetFullPathNameA(runtime_dir.c_str(), MAX_PATH, abs_path, NULL);
        _abs_runtime = abs_path;
        SetDllDirectoryA(_abs_runtime.c_str());
        int wlen = MultiByteToWideChar(CP_UTF8, 0, _abs_runtime.c_str(), -1, NULL, 0);
        std::wstring wpath(wlen, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, _abs_runtime.c_str(), -1, &wpath[0], wlen);
        static std::wstring s_python_home = wpath;
        Py_SetPythonHome(s_python_home.c_str());
        std::wstring w_python_exe = wpath + L"\\python.exe";
        static std::wstring s_python_exe = w_python_exe;
        Py_SetProgramName(s_python_exe.c_str());
#else
        _abs_runtime = runtime_dir;
        setenv("PYTHONHOME", runtime_dir.c_str(), 1);
#endif
    }
std::string EmbeddedPython::runtime_path() { return _abs_runtime; }

std::string EmbeddedPython::_abs_runtime;

std::vector<std::vector<double>> parseRanges(const std::string& json)
{
    std::vector<std::vector<double>> ranges;
    std::string s = json;

    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return ranges;
    size_t last = s.find_last_not_of(" \t\r\n");
    s = s.substr(first, last - first + 1);

    if (s.size() >= 2 && s.front() == '[' && s.back() == ']') {
        s = s.substr(1, s.size() - 2);
    }

    size_t start = 0;
    while (start < s.size()) {
        size_t lb = s.find('[', start);
        if (lb == std::string::npos) break;
        size_t rb = s.find(']', lb);
        if (rb == std::string::npos) break;

        std::string pair = s.substr(lb + 1, rb - lb - 1);
        while (!pair.empty() && pair.front() == '[') pair.erase(pair.begin());

        size_t comma = pair.find(',');
        if (comma != std::string::npos) {
            try {
                double min = std::stod(pair.substr(0, comma));
                double max = std::stod(pair.substr(comma + 1));
                ranges.push_back({ min, max });
            }
            catch (...) {
                std::cerr << "Failed to parse range: " << pair << std::endl;
                return {};
            }
        }
        start = rb + 1;
        while (start < s.size() && (s[start] == ',' || std::isspace(static_cast<unsigned char>(s[start])))) ++start;
    }
    return ranges;
}
std::vector<std::vector<double>> g_lastPopulation;
std::string readLineFromStdin()
{
    std::string line;
    std::getline(std::cin, line);
    return line;
}

void sendMessage(const std::string& json)
{
    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QString::fromStdString(json));
    //std::cout << json << std::endl;
}

std::string populationValuesToJson(const py::array_t<double>& pop)
{
    std::string json = "[";
    py::ssize_t N = pop.shape(0);
    py::ssize_t D = (pop.ndim() > 1) ? pop.shape(1) : 1;

    for (py::ssize_t k = 0; k < N; ++k) {
        if (k > 0) json += ",";
        json += "[";
        for (py::ssize_t j = 0; j < D; ++j) {
            if (j > 0) json += ",";
            double val = (pop.ndim() > 1) ? pop.at(k, j) : pop.at(k);
            json += std::to_string(val);
        }
        json += "]";
    }
    json += "]";
    return json;
}

std::string populationToJson(const py::array_t<double>& pop)
{
    return "{\"type\":\"population\",\"values\":" + populationValuesToJson(pop) + "}";
}


std::vector<double> parseArray(const std::string& line, const std::string& key)
{
    std::vector<double> values;
    size_t pos = line.find("\"" + key + "\"");
    if (pos == std::string::npos) return values;

    size_t lb = line.find('[', pos);
    if (lb == std::string::npos) return values;
    size_t rb = line.find(']', lb);
    if (rb == std::string::npos) return values;

    std::string content = line.substr(lb + 1, rb - lb - 1);
    size_t start = 0;
    while (start < content.size()) {
        size_t comma = content.find(',', start);
        std::string token;
        if (comma == std::string::npos) {
            token = content.substr(start);
            start = content.size();
        }
        else {
            token = content.substr(start, comma - start);
            start = comma + 1;
        }

        while (!token.empty() && std::isspace(static_cast<unsigned char>(token.back()))) token.pop_back();
        while (!token.empty() && std::isspace(static_cast<unsigned char>(token.front()))) token.erase(token.begin());
        if (!token.empty()) {
            try {
                values.push_back(std::stod(token));
            }
            catch (...) {
            }
        }
    }
    return values;
}

py::array_t<double> vectorToArray(const std::vector<double>& v)
{
    py::array_t<double> arr(v.size());
    for (size_t i = 0; i < v.size(); ++i) {
        arr.mutable_at(i) = v[i];
    }
    return arr;
}

py::array_t<double> vectorToArray2D(const std::vector<double>& v, size_t rows, size_t cols)
{
    if (cols == 0) {
        return py::array_t<double>(0);
    }
    py::ssize_t r = static_cast<py::ssize_t>(rows);
    py::ssize_t c = static_cast<py::ssize_t>(cols);
    py::array_t<double> arr({ r, c });
    for (size_t i = 0; i < v.size() && i < rows * cols; ++i) {
        arr.mutable_at(
            static_cast<py::ssize_t>(i / cols),
            static_cast<py::ssize_t>(i % cols)) = v[i];
    }
    return arr;
}

MxOptionTest::MxOptionTest() {
    auto pyPath = qApp->applicationDirPath() + "/python";
    if (!QDir(pyPath).exists())
    {
        pyPath = qApp->applicationDirPath() + "/../tools/python";
    }
    runtime = pyPath.toStdString();
    pythonLib = qApp->applicationDirPath().toStdString();
}
MxOptionTest::~MxOptionTest() {

}
std::shared_ptr<MxOptionTest> MxOptionTest::getInstance() {
    static std::shared_ptr<MxOptionTest> instance = nullptr;
    if (instance == nullptr)
        instance = std::make_shared<MxOptionTest>();
    return instance;
}

int MxOptionTest::OptiomTest(std::string configJson)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    try {
        EmbeddedPython epy(runtime);
        //py::scoped_interpreter guard{};

        py::module sys = py::module_::import("sys");
        sys.attr("path").attr("append")(pythonLib);

        py::module jsonMod = py::module_::import("json");
        py::object config = jsonMod.attr("loads")(configJson);

        std::string algorithm = config["algorithm"].cast<std::string>();
        py::dict hyperParams = config["hyperparams"].cast<py::dict>();
        py::list rangesList = config["ranges"].cast<py::list>();

        std::vector<std::vector<double>> ranges;
        for (auto item : rangesList) {
            py::list bound = item.cast<py::list>();
            std::vector<double> range;
            for (auto v : bound) {
                range.push_back(v.cast<double>());
            }
            ranges.push_back(range);
        }

        // 统一超参数
        py::dict hp;
        auto copyInt = [&](const char* key, int defaultVal) {
            if (hyperParams.contains(key)) hp[key] = hyperParams[key];
            else hp[key] = defaultVal;
            };
        auto copyDouble = [&](const char* key, double defaultVal) {
            if (hyperParams.contains(key)) hp[key] = hyperParams[key];
            else hp[key] = defaultVal;
            };

        copyInt("pop_size", 10);
        copyInt("max_iterations", 5);
        copyInt("seed", 42);
        copyInt("n_constraints", 0);
        copyInt("n_objectives", 1);
        copyDouble("w", 0.8);
        copyDouble("c1", 2.0);
        copyDouble("c2", 2.0);
        copyDouble("penalty_factor", 1e6);
        copyDouble("tol", 1e-6);

        // UI 输入的 max_iterations 视为总代数（包含初始种群）
        int totalGenerations = 5;
        try {
            totalGenerations = hp["max_iterations"].cast<int>();
        }
        catch (...) {}
        if (totalGenerations < 1) totalGenerations = 1;
        // 优化器内部的 max_iterations 是 update 次数
        hp["max_iterations"] = totalGenerations - 1;

        py::object optimizer = py::module_::import("mxsim_optim.optimization")
            .attr("OptimizerFactory").attr("create")(algorithm, ranges, hp);

        // 发送初始种群
        py::array_t<double> pop = optimizer.attr("initialize")().cast<py::array_t<double>>();
        sendMessage(populationToJson(pop));


        // 外部循环：剩余 totalGenerations - 1 代
        bool doneSent = false;
        for (int iter = 0; iter < totalGenerations - 1; ++iter) {
            std::string line = readLineFromStdin();
            if (line.empty()) break;

            std::vector<double> f = parseArray(line, "objective");
            std::vector<double> g = parseArray(line, "constraint");

            if (f.empty()) break;

            py::array_t<double> fArr = vectorToArray(f);
            py::array_t<double> gArr;
            int nConstraints = optimizer.attr("n_constraints").cast<int>();
            if (nConstraints > 0) {
                gArr = vectorToArray2D(g, f.size(), nConstraints);
            }
            else {
                gArr = vectorToArray(g);
            }

            pop = optimizer.attr("update")(fArr, gArr).cast<py::array_t<double>>();

            if (optimizer.attr("is_converged")().cast<bool>()) {
                sendMessage("{\"type\":\"done\",\"values\":" + populationValuesToJson(pop) + "}");
                doneSent = true;
                break;
            }

            sendMessage(populationToJson(pop));
        }


        if (!doneSent) {
            sendMessage("{\"type\":\"done\",\"values\":" + populationValuesToJson(pop) + "}");
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}


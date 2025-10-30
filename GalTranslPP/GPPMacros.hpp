#ifdef PYBIND11_HEADERS 
#define PYBIND11_DETAILED_ERROR_MESSAGES
#include <pybind11/stl.h>
#include <pybind11/complex.h>
#include <pybind11/functional.h>
#include <pybind11/stl_bind.h>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#endif

#define NESTED_CVT(className, memberName) sol::property([](className& self, sol::this_state s) \
{ \
	return sol::nested<decltype(className::memberName)>(self.memberName); \
}, [](className& self, sol::this_state s, decltype(className::memberName) table) { self.memberName = std::move(table); }) 

// 为 std::filesystem::path 提供 pybind11 类型转换器
#define PATH_CVT                                                                    \
namespace pybind11 {                                                                \
    namespace detail {                                                              \
        template <>                                                                 \
        struct type_caster<fs::path> {                                              \
        public:                                                                     \
        /*PYBIND11_TYPE_CASTER 宏定义了 boilerplate 代码，比如 value 成员变量*/     \
        protected: fs::path value; public: static constexpr auto name = const_name("os.PathLike"); template <typename T_, ::pybind11::detail::enable_if_t< std::is_same<fs::path, ::pybind11::detail::remove_cv_t<T_>>::value, int> = 0> static ::pybind11::handle cast(T_* src, ::pybind11::return_value_policy policy, ::pybind11::handle parent) {\
            if (!src) return ::pybind11::none().release(); if (policy == ::pybind11::return_value_policy::take_ownership) {\
                auto h = cast(std::move(*src), policy, parent); delete src; return h;\
            } return cast(*src, policy, parent);\
        } operator fs::path* () {\
            return &value;\
        } operator fs::path& () {\
            return value;\
        } operator fs::path && ()&& {\
            return std::move(value);\
        } template <typename T_> using cast_op_type = ::pybind11::detail::movable_cast_op_type<T_>;\
            /*load 函数负责从 Python 对象中读取数据，并将其转换为 C++ 类型*/        \
            bool load(handle src, bool) {                                           \
                /*检查输入是否是 Python 的 str 或 pathlib.Path 对象*/               \
                if (py::isinstance<py::str>(src)) {                                 \
                    /*如果是字符串，直接转换*/                                      \
                    value = fs::path(ascii2Wide(src.cast<std::string>()));          \
                    return true;                                                    \
                }                                                                   \
                /*尝试将对象当作 os.PathLike (比如 pathlib.Path) 处理*/             \
                /*通过调用 str() 将其转换为字符串路径*/                             \
                /*py::str(src) 会调用对象的 __str__ 方法*/                          \
                try {                                                               \
                    std::string path_str = py::str(src);                            \
                    value = fs::path(ascii2Wide(path_str));                         \
                    return true;                                                    \
                }                                                                   \
                catch (const py::error_already_set&) {                              \
                    return false;                                                   \
                }                                                                   \
            }                                                                       \
            /*2. C++ -> Python 的转换*/                                             \
            /*当 C++ 函数返回 fs::path 时，这个 cast 函数会被调用*/                 \
            static handle cast(const fs::path& src, return_value_policy, handle) {  \
                /*将 C++ 的 fs::path 转换为 Python 的 pathlib.Path 对象*/           \
                /*首先，将 fs::path 转换为 UTF-8 字符串*/                           \
                std::string pathStr = wide2Ascii(src.wstring());                    \
                /*导入 Python 的 pathlib 模块*/                                     \
                py::module_ pathlib = py::module_::import("pathlib");               \
                /*创建一个 pathlib.Path 对象*/                                      \
                py::object pyPath = pathlib.attr("Path")(pathStr);                  \
                /*detach() 的作用是增加 Python 对象的引用计数，*/                   \
                /*防止在我们返回 handle 后它被立即销毁。*/                          \
                return pyPath.release();                                            \
            }                                                                       \
        };                                                                          \
    }                                                                               \
} // namespace pybind11::detail
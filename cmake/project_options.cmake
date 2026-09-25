# 定义一个 CMake 函数，函数名为 storage_set_project_options
# 调用时需要传入一个 target 名称，例如 storage_core

function(storage_set_project_options target)
    # 要求当前 target 使用 C++20。
    # PUBLIC 表示：
    # 1. 当前 target 自己使用 C++20
    # 2. 依赖当前 target 的其他 target 也会继承这个 C++20 要求
    target_compile_features(${target} PUBLIC cxx_std_20)

    # 给当前 target 添加编译警告选项。
    # PRIVATE 表示这些编译选项只作用于当前 target，
    # 不会传递给依赖它的其他 target。
    #
    # -Wall      ：开启一批常用编译警告
    # -Wextra    ：开启额外的警告
    # -Wpedantic ：严格检查不符合标准 C++ 的写法
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)


    if(STORAGE_ENABLE_SANITIZERS)

        # 给“编译阶段”加入 Sanitizer 参数。
        #
        # -fsanitize=address
        # → 开启 AddressSanitizer（ASan），
        #   用于检测越界访问、Use After Free、内存泄漏等问题。
        #
        # -fsanitize=undefined
        # → 开启 UndefinedBehaviorSanitizer（UBSan），
        #   用于检测整数溢出、非法移位、错误类型转换等未定义行为。
        #
        # -fno-omit-frame-pointer
        # → 不省略栈帧指针，
        #   方便 Sanitizer 输出更完整、更容易阅读的调用栈。
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        # 给“链接阶段”也加入 ASan 和 UBSan。
        #
        # 仅仅在编译阶段添加 -fsanitize 还不够，
        # 最终生成可执行文件/动态库时还需要链接 Sanitizer Runtime。
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
endfunction()


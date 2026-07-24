import os
import sys
import platform
import shutil


def run_cmake_on_linux(build_type: str, version: str = "99.99.99", company: str = "", description: str = ""):
    if version == "":
        version = "99.99.99"
    build_command = f"""
        mkdir -p build &&
        cd build &&
        cmake .. -DCMAKE_BUILD_TYPE={build_type} -D VERSION_ARG="{version}" -D COMPANY_ARG="{company}" -D DESCRIPTION_ARG={description} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON &&
        ln -sf build/compile_commands.json ../ &&
        make -j$(nproc) > build.log 2>&1

        """
    if shutil.which("ninja") is not None:
        build_command = f"""
        mkdir -p build &&
        cd build &&
        cmake .. -DCMAKE_BUILD_TYPE={build_type} -G Ninja -D VERSION_ARG="{version}" -D COMPANY_ARG="{company}" -D DESCRIPTION_ARG={description} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON &&
        ln -sf build/compile_commands.json ../ &&
        ninja > build.log 2>&1
        """

    status = os.system(build_command)
    if status != 0:
        exit_code = os.waitstatus_to_exitcode(status)
        sys.exit(exit_code)
    sys.exit(0)


def run_cmake_on_windows(build_type: str, version: str = "99.99.99", company: str = "", description: str = ""):
    if version == "":
        version = "99.99.99"

    pre_build_command = f"""
    if not exist build mkdir build;
    """
    os.system(pre_build_command)

    build_command = f"""
        cd build && cmake .. -D VERSION_ARG="{version}"
        """

    if shutil.which("ninja") is not None:
        build_command = f"""
        cd build && cmake .. -G Ninja -DCMAKE_BUILD_TYPE={build_type} -DVERSION_ARG="{version}" -D COMPANY_ARG="{company}" -D DESCRIPTION_ARG={description} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && ninja > build.log 2>&1
        """
    elif shutil.which("make") is not None:
        build_command = f"""
        cd build && cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE={build_type} -DVERSION_ARG="{version}" -D COMPANY_ARG="{company}" -D DESCRIPTION_ARG={description} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && make -j $env:NUMBER_OF_PROCESSORS > build.log 2>&1
        """

    else:
        print("Ninja not found. Will produce Visual Studio Solution. Please open in Visual Studio or compile it with MSBuild")

    status = os.system(build_command)

    if status != 0:
        exit_code = os.waitstatus_to_exitcode(status)
        sys.exit(exit_code)

    sys.exit(0)


def run_app(build_type: str, version: str = "", company: str = "", description: str = ""):
    operating_system = platform.system()
    match operating_system:
        case "Linux":
            print("Linux detected, let us run the build.")
            run_cmake_on_linux(build_type, version, company, description)
        case "Windows":
            print("Windows detected, let us generate a Visual Studio Solution file.")
            run_cmake_on_windows(build_type, version, company, description)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Welcome to AhoiCpp build process. Pick your build or just press enter to go fast.")
        print("[Enter]  FastBuild")
        print("[0]      Release")
        print("[1]      Debug")
        build_selection = input()

        match(build_selection):
            case "":
                run_app("Release", "")
            case "0":
                print("You selected Release. Please add a version to your build. Format recommended: XX.XX.XX. Hint: you can just press enter and leave it empty.")
                version = input()
                run_app("Release", version)
            case "1":
                print("Build as Debug")
                run_app("RelWithDebInfo", "")
            case _:
                print("Invalid input. Bye.")
    elif (sys.argv[1] == "debug"):
        version = sys.argv[2] if len(sys.argv) > 2 else "99.99.99"
        company = sys.argv[3] if len(sys.argv) > 3 else "Ahoi Labs"
        description = sys.argv[4] if len(
            sys.argv) > 4 else "Your project is owned by Ahoi Labs."
        run_app("RelWithDebInfo", version, company, description)
    else:
        version = sys.argv[2] if len(sys.argv) > 2 else "99.99.99"
        company = sys.argv[3] if len(sys.argv) > 3 else "Ahoi Labs"
        description = sys.argv[4] if len(
            sys.argv) > 4 else "Your project is owned by Ahoi Labs."
        run_app("Release", version, company, description)

import os.path
import re


def walk_excluding(directory, exclude_dirs):
    exclude_dirs = {os.path.normpath(d) for d in exclude_dirs}  # 规范化路径

    for root, dirs, files in os.walk(directory):
        if any(os.path.normpath(root).startswith(exclude) for exclude in exclude_dirs):
            dirs[:] = []
            continue
        dirs[:] = [d for d in dirs if d not in {os.path.basename(ex) for ex in exclude_dirs}]
        yield root, dirs, files


def find_header_files(directory, excludes_abs):
    header_files = []
    for root, dirs, files in walk_excluding(directory, excludes_abs):
        dirs[:] = [d for d in dirs if d not in excludes_abs]
        for file in files:
            if file.endswith(".h"):
                header_files.append(os.path.join(root, file))
    return header_files


def remove_cpp_keywords(line_content):
    return line_content.replace('static ', '').replace('virtual ', '')


def param_convert_cpp(c_param_type, c_param_value):
    param_type = c_param_type
    if "const std::string&" in c_param_type or "std::string" in c_param_type:
        param_type = 'char*'
    if 'const void*' in c_param_type:
        param_type = 'void*'
    if 'Ptr' in c_param_type:
        param_type = c_param_type.replace('Ptr', '*')
        c_param_value = c_param_value + '_sp'
        # print(f"{c_param_type} smart pointer is not allowed")
        # assert False
    return param_type, c_param_value


def param_convert_js(c_param_type):
    js_param_type = c_param_type
    if '*' in c_param_type and 'char*' not in c_param_type:
        js_param_type = 'void*'
    elif 'int' in c_param_type or 'size_t' in c_param_type:
        js_param_type = 'int'
    elif 'Layer2D' in c_param_type or 'LightType' in c_param_type or 'BlendMode' in c_param_type:
        js_param_type = 'int'
    elif 'Ptr' in c_param_type:
        js_param_type = 'void*'
    elif "const std::string&" in c_param_type or "std::string" in c_param_type:
        js_param_type = 'char*'
    return js_param_type


def gen_js_file(js_file, return_type, func_name, params_list, namespace):
    if '*' in return_type and 'char*' not in return_type:
        return_type = 'void*'
    if 'Ptr' in return_type:
        return_type = 'void*'
    c_params_simple_str = ''
    c_params_value_str = ''
    for c_param in params_list:
        idx = c_param.rfind(' ') + 1
        param_type = c_param[:idx].strip()
        param_value = c_param[idx:].strip()
        if '*' in param_value:
            param_type += '*'
            param_value.removeprefix('*')
        if '&' in param_value:
            param_type += '&'
            param_value = param_value[1:]
        param_type = param_convert_js(param_type)
        c_params_simple_str += param_type + ', '
        c_params_value_str += param_value + ', '
    c_params_simple_str = c_params_simple_str[:-2]
    c_params_value_str = c_params_value_str[:-2]
    if len(params_list) == 0:
        js_file.write(f'{func_name}: function() {{\n')
    else:
        js_file.write(f'{func_name}: function({c_params_value_str}) {{\n')
    js_file.write(f'    let f = ffi(\"{return_type} {func_name}({c_params_simple_str})\");\n')
    js_file.write(f'    return f({c_params_value_str});\n')
    if len(namespace) > 0:
        js_file.write('},\n')
    else:
        js_file.write('}\n')


def get_vector_param(vector_len):
    if vector_len == 2:
        return 'Vector2f(vec_x, vec_y), '
    elif vector_len == 3:
        return 'Vector3f(vec_x, vec_y, vec_z), '
    else:
        print("Vector type not support!")
        return ""


def gen_cpp_file(cpp_file, class_name, static_func, return_type, func_name, params_list):
    if len(class_name) == 0:
        return
    if 'Ptr' in return_type:
        return_type = return_type.replace('Ptr', '*')
    if not static_func:
        class_ptr = params_list[0].split(' ')[1]
    func_name_origin = func_name[func_name.find('_') + 1:]
    c_params_format_str = ''
    c_call_params_str = ''
    vector_len = 0
    for i in range(len(params_list)):
        c_param = params_list[i]
        idx = c_param.rfind(' ') + 1
        param_type = c_param[:idx].strip()
        param_value = c_param[idx:].strip()
        if param_value.startswith('*'):
            param_type += '*'
            param_value.removeprefix('*')
        if param_value.startswith('&'):
            param_type += '&'
            param_value = param_value[1:]
        param_type, param_value = param_convert_cpp(param_type, param_value)
        c_params_format_str += f'{param_type} {param_value}, '
        # Ignore first param which is class pointer
        if i > 0 or static_func:
            # convert float x, float y,( float z)  to Vector2f (Vector3f)
            if 'vec_' in param_value and 'float' in param_type:
                vector_len += 1
                if i == len(params_list) - 1:
                    c_call_params_str += get_vector_param(vector_len)
            else:
                if vector_len > 0:
                    c_call_params_str += get_vector_param(vector_len)
                    vector_len = 0
                if param_type.endswith('*') and param_value.endswith('_sp'):
                    c_call_params_str += f'{param_type.removesuffix("*")}Ptr({param_value}), '
                else:
                    c_call_params_str += f'{param_value}, '
    c_params_format_str = c_params_format_str[:-2]
    c_call_params_str = c_call_params_str[:-2]
    if len(params_list) == 0:
        cpp_file.write(f'{return_type} {func_name}() {{\n')
    else:
        cpp_file.write(f'{return_type} {func_name}({c_params_format_str}) {{\n')
    # write class body
    return_str = ''
    if 'void' not in return_type:
        return_str = 'return '
    if static_func:
        cpp_file.write(f'    {return_str}{class_name}::{func_name_origin}({c_call_params_str});\n')
    else:
        cpp_file.write(f'    {return_str}{class_ptr}->{func_name_origin}({c_call_params_str});\n')
    cpp_file.write('}\n')


def api_binding_generator(header_path: str, header_excludes: list, js_out_path: str, cpp_out_path: str, api_name: str, namespace: str):
    script_dir = os.path.dirname(os.path.abspath(__file__))
    if not js_out_path.endswith('.js'):
        js_out_path += f'/{api_name}.js'
    if not cpp_out_path.startswith('/'):
        cpp_out_path = os.path.join(script_dir, cpp_out_path)
    if not os.path.exists(cpp_out_path):
        os.makedirs(cpp_out_path)
    cpp_out_file = cpp_out_path + f'/{api_name}_export.cpp'
    if not js_out_path.startswith('/'):
        js_out_path = os.path.join(script_dir, js_out_path)
    if not header_path.startswith('/'):
        header_path = os.path.normpath(os.path.join(script_dir, header_path))
    with open(js_out_path, 'w', encoding='utf-8') as js_file:
        if len(namespace) > 0:
            js_file.write(f'let {namespace} = {{\n')
        with open(cpp_out_file, 'w', encoding='utf-8') as cpp_file:
            excludes_abs = []
            for exclude in header_excludes:
                excludes_abs.append(os.path.normpath(os.path.join(script_dir, exclude)))
            headers = find_header_files(header_path, excludes_abs)
            export_headers = set()
            export_funcs = set()
            for header in headers:
                with open(header, 'r', encoding='utf-8') as h_file:
                    begin = False
                    comment = False
                    class_name = ""
                    for line in h_file.readlines():
                        line_content = line.strip()
                        if 'class ' in line_content and 'friend class' not in line_content:
                            find_class = re.findall(r'class (\w+)', line_content)
                            if len(find_class) > 0:
                                class_name = find_class[0]
                        if '[JS_BINDING_BEGIN]' in line_content:
                            begin = True
                            export_headers.add(header)
                        elif '[JS_BINDING_END]' in line_content:
                            begin = False
                        elif begin:
                            if '//' in line_content or ('/*' in line_content and '*/' in line_content) or ('#' in line_content):
                                continue
                            if '/*' in line_content:
                                comment = True
                                continue
                            if '*/' in line_content:
                                comment = False
                                continue
                            if comment:
                                continue
                            if ' = delete;' in line_content:
                                continue
                            split_index = line_content.find('(')
                            remove_str = line_content[split_index + 1:]
                            while '(' in remove_str:
                                left_bracket_index = remove_str.find('(')
                                right_bracket_index = remove_str.find(')')
                                remove_str = remove_str[:left_bracket_index] + remove_str[right_bracket_index + 1:]
                            line_content = line_content[:split_index + 1] + remove_str
                            # remove parameter default values
                            line_content = re.sub(r"\s*=\s*[^,)]+", "", line_content)
                            static_func = 'static ' in line_content
                            line_content = remove_cpp_keywords(line_content)
                            parts = line_content.split(' ')
                            if len(parts) < 2:
                                continue
                            return_type = parts[0]
                            func_name = parts[1].split('(')[0]
                            if '*' in func_name:
                                return_type += '*'
                                func_name = func_name.removeprefix('*')
                            # find parameters
                            pattern = r"\((.*?)\)"
                            finds = re.findall(pattern, line_content)
                            if len(finds) == 0:
                                print(f"Only function declaration can be export! line:{line_content}")
                                assert False
                            c_params_str = finds[0]
                            c_params_list = c_params_str.split(',')
                            if len(class_name) > 0 and not static_func:
                                # Set object pointer as first parameter
                                func_name = class_name + '_' + func_name
                                c_params_list.insert(0, f'{class_name}* ptr')
                            transformed_params = []
                            for c_param in c_params_list:
                                clear_param = c_param.strip()
                                if len(clear_param) == 0:
                                    continue
                                # remove default value
                                if '=' in clear_param:
                                    clear_param = clear_param[:clear_param.find('=')].strip()
                                if 'Vector2' in clear_param:
                                    transformed_params.append('float vec_x')
                                    transformed_params.append('float vec_y')
                                elif 'Vector3' in clear_param:
                                    transformed_params.append('float vec_x')
                                    transformed_params.append('float vec_y')
                                    transformed_params.append('float vec_z')
                                else:
                                    transformed_params.append(clear_param)
                            gen_js_file(js_file, return_type, func_name, transformed_params, namespace)
                            gen_cpp_file(cpp_file, class_name, static_func, return_type, func_name, transformed_params)
                            export_funcs.add(func_name)
        with open(cpp_out_file, 'r', encoding='utf-8') as file:
            existing_content = file.read()
        # 在文件开头添加新内容
        content = '// Generated by js_api_generator.py\n'
        content += "#include <unordered_map>\n"
        for header in export_headers:
            relative_path = os.path.relpath(header, os.path.dirname(cpp_out_file))
            content += f'#include "{relative_path}"\n'
        content += "extern std::unordered_map<uint32_t, void*>* g_APIMap;\n"
        content = content + existing_content
        content += f"void Register_{api_name.upper()}() {{\n"
        content += "    std::unordered_map<uint32_t, void*>& apiMap = *g_APIMap;\n"
        for func in export_funcs:
            content += f'    apiMap[std::hash<std::string>()("{func}")] = (void*){func};\n'
        content += "}\n"
        # 将新内容写回文件
        with open(cpp_out_file, 'w', encoding='utf-8') as file:
            file.write(content)
        if len(namespace) > 0:
            js_file.write('};\n')


def main():
    js_out_path = '../../../spiffs_img'
    absolute_path = os.path.abspath(js_out_path)
    if not os.path.exists(absolute_path):
        print(f"Javascript output path: {absolute_path} not exist, JS Binding export abort.")
        return
    # export spine library
    header_path = '../cubicat-port'
    header_excludes = ['../dist']
    cpp_out_path = '../js_binding'
    api_binding_generator(header_path, header_excludes, js_out_path, cpp_out_path, 'spine_api', 'spine')

if __name__ == '__main__':
    main()

"""
mbed SDK
Copyright (c) 2011-2013 ARM Limited

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""
import sys
from os.path import join, abspath, dirname, exists
ROOT = abspath(join(dirname(__file__), ".."))
sys.path.append(ROOT)

from shutil import move

from workspace_tools.paths import *
from workspace_tools.utils import mkdir, cmd
from workspace_tools.export import export, setup_user_prj


USR_PRJ_NAME = "usr_prj"
USER_PRJ = join(EXPORT_WORKSPACE, USR_PRJ_NAME)
USER_SRC = join(USER_PRJ, "src")


def setup_test_user_prj():
    if exists(USER_PRJ):
        print 'Test user project already generated...'
        return
    
    setup_user_prj(USER_PRJ, join(TEST_DIR, "rtos", "mbed", "basic"), [join(LIB_DIR, "rtos")])
    
    # FAKE BUILD URL
    open(join(USER_SRC, "mbed.bld"), 'w').write("http://mbed.org/users/mbed_official/code/mbed/builds/976df7c37ad5\n")


def fake_build_url_resolver(url):
    # FAKE BUILD URL: Ignore the URL, always return the path to the mbed library
    return {'path':MBED_LIBRARIES, 'name':'mbed'}


def test_export(toolchain, target, expected_error=None):
    if toolchain is None and target is None:
        base_dir = join(EXPORT_TMP, "zip")
    else:
        base_dir = join(EXPORT_TMP, toolchain, target)
    temp_dir = join(base_dir, "temp")
    mkdir(temp_dir)
    
    zip_path, report = export(USER_PRJ, USR_PRJ_NAME, toolchain, target, base_dir, temp_dir, False, fake_build_url_resolver)
    
    if report['success']:
        move(zip_path, join(EXPORT_DIR, "export_%s_%s.zip" % (toolchain, target)))
        print "[OK]"
    else:
        if expected_error is None:
            print '[ERRROR] %s' % report['errormsg']
        else:
            if (zip_path is None) and (expected_error in report['errormsg']):
                print '[OK]'
            else:
                print '[ERROR]'
                print '    zip:', zip_path
                print '    msg:', report['errormsg']


if __name__ == '__main__':
    setup_test_user_prj()
    
    for toolchain, target in [
            ('uvision', 'LPC1768'), ('uvision', 'LPC11U24'), ('uvision', 'KL25Z'), ('uvision', 'LPC1347'), ('uvision', 'LPC1114'), ('uvision', 'LPC4088'),
            ('uvision', 'NUCLEO_F103RB'),
            
            ('codered', 'LPC1768'), ('codered', 'LPC4088'),
            
            # Linux path: /home/emimon01/bin/gcc-cs/bin/
            # Windows path: "C:/Program Files (x86)/CodeSourcery/Sourcery_CodeBench_Lite_for_ARM_EABI/bin/"
            ('codesourcery', 'LPC1768'),
            
            # Linux path: /home/emimon01/bin/gcc-arm/bin/
            # Windows path: C:/arm-none-eabi-gcc-4_7/bin/
            ('gcc_arm', 'LPC1768'),
            
            ('ds5_5', 'LPC1768'), ('ds5_5', 'LPC11U24'),
            
            ('iar', 'LPC1768'),
            
            (None, None)
        ]:
        print '\n=== Exporting to "%s::%s" ===' % (toolchain, target)
        test_export(toolchain, target)
    
    print "\n=== Test error messages ==="
    test_export('codered', 'LPC11U24', expected_error='codered')

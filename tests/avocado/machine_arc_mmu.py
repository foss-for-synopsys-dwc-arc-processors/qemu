# Tests for ARC MMU
#

from avocado_qemu import QemuSystemTest
from avocado_qemu import wait_for_console_pattern

class ARCMMUTestBase(QemuSystemTest):

    def __setup__(self):
        self.vm.add_args("-singlestep")
        self.vm.add_args('-nographic')
        self.vm.add_args('-serial', "stdio")
        self.vm.add_args('-no-reboot')
        self.vm.add_args('-m', '3G')
        self.vm.add_args('-display', 'none')
        self.vm.add_args("-monitor","none")
        # No monitoring required, just raw output
        self.vm.set_qmp_monitor(enabled=False)

    def __run__(self, url, hash):
        kernel_path_dnld = self.fetch_asset(url, hash)
        self.vm.add_args('-kernel', kernel_path_dnld)
        
        self.vm.launch()

        self.vm.wait()
        #self.vm.shutdown()

        if "[PASS]" not in self.vm._iolog:
            self.fail("Unexpected output: "+self.vm._iolog)

class ARCMMUTest(ARCMMUTestBase):
    # Set test timeout
    timeout = 2
    def test0(self):
        """
        :avocado: tags=arch:arc64
        :avocado: tags=machine:arc-sim
        """
        self.__setup__()

        self.vm.add_args("-cpu","hs6x,mmuv6-version=48_4k")
        
        hash = '2c0941e00e5ff5324195a81d7646f663fa3f8bd3'
        url = "https://raw.githubusercontent.com/foss-for-synopsys-dwc-arc-processors/arc-gnu-toolchain/master/test-qemu/images/MMU_Tests/mmuv48_00.exe"

        self.__run__(url, hash)

    def test1(self):
        """
        :avocado: tags=arch:arc64
        :avocado: tags=machine:arc-sim
        """
        self.__setup__()

        self.vm.add_args("-cpu","hs6x,mmuv6-version=48_4k")

        hash = 'edffe3ee3f0f27fb86f83897c9abe470b87e3558'
        url = "https://raw.githubusercontent.com/foss-for-synopsys-dwc-arc-processors/arc-gnu-toolchain/master/test-qemu/images/MMU_Tests/mmuv48_01.exe"

        self.__run__(url, hash)

    def test2(self):
        """
        :avocado: tags=arch:arc64
        :avocado: tags=machine:arc-sim
        """
        self.__setup__()

        self.vm.add_args("-cpu","hs6x,mmuv6-version=48_4k")

        hash = 'a071d3a059c8dc1933b16a956d7892d3003dea52'
        url = "https://raw.githubusercontent.com/foss-for-synopsys-dwc-arc-processors/arc-gnu-toolchain/master/test-qemu/images/MMU_Tests/mmuv48_02.exe"

        self.__run__(url, hash)

    def test3(self):
        """
        :avocado: tags=arch:arc64
        :avocado: tags=machine:arc-sim
        """
        self.__setup__()

        self.vm.add_args("-cpu","hs6x,mmuv6-version=48_4k")

        hash = '11e09bf6f40f9a3930ffae63d8e5a4b2d6a7cd08'
        url = "https://raw.githubusercontent.com/foss-for-synopsys-dwc-arc-processors/arc-gnu-toolchain/master/test-qemu/images/MMU_Tests/mmuv48_03.exe"

        self.__run__(url, hash)

    def test4(self):
        """
        :avocado: tags=arch:arc64
        :avocado: tags=machine:arc-sim
        """
        self.__setup__()

        self.vm.add_args("-cpu","hs6x,mmuv6-version=48_16k")

        hash = '953672f86166ff49c16210f89451ed6ddfc6eb13'
        url = "https://raw.githubusercontent.com/foss-for-synopsys-dwc-arc-processors/arc-gnu-toolchain/master/test-qemu/images/MMU_Tests/mmuv48_04.exe"

        self.__run__(url, hash)

    def test5(self):
        """
        :avocado: tags=arch:arc64
        :avocado: tags=machine:arc-sim
        """
        self.__setup__()

        self.vm.add_args("-cpu","hs6x,mmuv6-version=48_64k")

        hash = '468f3aca0831a3e757ad7ca7fa113b69ad96a4b8'
        url = "https://raw.githubusercontent.com/foss-for-synopsys-dwc-arc-processors/arc-gnu-toolchain/master/test-qemu/images/MMU_Tests/mmuv48_05.exe"

        self.__run__(url, hash)

    # Update TLB sequence (QEMU does not currently support this)
    def test6(self):
        """
        :avocado: tags=arch:arc64
        :avocado: tags=machine:arc-sim
        """
        self.__setup__()

        self.vm.add_args("-cpu","hs6x,mmuv6-version=48_4k")

        hash = '435b2d645cfe080adf49c4e423a8928bcad89376'
        url = "https://raw.githubusercontent.com/foss-for-synopsys-dwc-arc-processors/arc-gnu-toolchain/master/test-qemu/images/MMU_Tests/mmuv48_06.exe"

        self.__run__(url, hash)


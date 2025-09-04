#include "pch.h"
//#include <vulkan/vulkan.hpp>
//#include <vulkan/vulkan_raii.hpp>
#include "core/engine.h"

int main() {
    Core& app = Core::GetInstance();

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

//int main() {
//    vk::raii::Context ctx;
//    vk::ApplicationInfo ai{ .sType = vk::StructureType::eApplicationInfo,
//        .pApplicationName = "The Wheel",
//        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
//        .pEngineName = "No Engine",
//        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
//        .apiVersion = vk::ApiVersion14 };
//    vk::InstanceCreateInfo ici{}; ici.setPApplicationInfo(&ai);
//    vk::raii::Instance inst{ ctx, ici };
//    auto pds = inst.enumeratePhysicalDevices();
//    std::cout << "Devices: " << pds.size() << "\n";
//    for (auto& pd : pds) {
//        auto props = pd.getProperties();
//        std::cout << " - " << props.deviceName << "\n";
//    }
//}

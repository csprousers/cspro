#pragma once

#include <zEngineO/zEngineO.h>
#include <zAppO/Application.h>

class ApplicationLoader;


ZENGINEO_API void BuildApplication(std::shared_ptr<ApplicationLoader> application_loader,
                                   std::optional<EngineAppType> required_application_type = std::nullopt);

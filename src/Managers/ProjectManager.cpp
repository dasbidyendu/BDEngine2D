#include "ProjectManager.h"
#include "../Utils/Logger.h"
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

ProjectManager::ProjectManager() {}

bool ProjectManager::CreateProject(const std::string &directory,
                                   const std::string &projectName) {
  fs::path targetDir = fs::path(directory) / projectName;

  try {
    if (!fs::exists(targetDir)) {
      fs::create_directories(targetDir);
    }

    fs::create_directories(targetDir / "assets" / "scenes");
    fs::create_directories(targetDir / "assets" / "scripts");
    fs::create_directories(targetDir / "assets" / "textures");
    fs::create_directories(targetDir / "assets" / "fonts");

    // Create default main.scene
    std::ofstream mainSceneFile(targetDir / "assets" / "scenes" / "main.scene");
    if (mainSceneFile.is_open()) {
      mainSceneFile << "END\n";
      mainSceneFile.close();
    }

    std::string beprojPath = (targetDir / (projectName + ".beproj")).string();

    m_activeProject.name = projectName;
    m_activeProject.path = beprojPath;
    m_activeProject.startScene = "assets/scenes/main.scene";

    SaveCurrentProject();

    Logger::AddLog(LOG_LEVEL_SUCCESS, "Created new project %s at %s",
                   projectName.c_str(), beprojPath.c_str());
    return true;
  } catch (const std::exception &e) {
    Logger::AddLog(LOG_LEVEL_ERROR, "Failed to create project %s: %s",
                   projectName.c_str(), e.what());
    return false;
  }
}

bool ProjectManager::LoadProject(const std::string &beprojPath) {
  if (!fs::exists(beprojPath)) {
    Logger::AddLog(LOG_LEVEL_ERROR, "Project file does not exist: %s",
                   beprojPath.c_str());
    return false;
  }

  std::ifstream file(beprojPath);
  if (!file.is_open()) {
    Logger::AddLog(LOG_LEVEL_ERROR, "Could not open project file: %s",
                   beprojPath.c_str());
    return false;
  }

  m_activeProject.path = beprojPath;
  m_activeProject.name =
      fs::path(beprojPath).stem().string(); // Default to filename if missing

  std::string line;
  while (std::getline(file, line)) {
    std::istringstream is_line(line);
    std::string key, value;
    if (std::getline(is_line, key, '=') && std::getline(is_line, value)) {
      if (key == "Name")
        m_activeProject.name = value;
      if (key == "StartScene")
        m_activeProject.startScene = value;
    }
  }
  file.close();

  // Change current working directory to the project directory
  // This removes the need for hardcoded "assets" everywhere
  // as it will become relative to the project root.
  fs::path projectDirectory = fs::path(beprojPath).parent_path();
  fs::current_path(projectDirectory);

  Logger::AddLog(LOG_LEVEL_SUCCESS, "Loaded project: %s",
                 m_activeProject.name.c_str());
  return true;
}

bool ProjectManager::SaveCurrentProject() {
  if (m_activeProject.path.empty())
    return false;

  std::ofstream file(m_activeProject.path);
  if (!file.is_open()) {
    Logger::AddLog(LOG_LEVEL_ERROR, "Failed to save project file: %s",
                   m_activeProject.path.c_str());
    return false;
  }

  file << "Name=" << m_activeProject.name << "\n";
  file << "StartScene=" << m_activeProject.startScene << "\n";
  file.close();

  return true;
}

void ProjectManager::CloseProject() {
  m_activeProject.name = "";
  m_activeProject.path = "";
  m_activeProject.startScene = "";
}

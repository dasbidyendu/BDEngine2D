#pragma once
#include <filesystem>
#include <string>
#include <vector>


struct Project {
  std::string name;
  std::string path; // Absolute path to the .beproj file
  std::string startScene = "assets/scenes/main.scene";
};

class ProjectManager {
public:
  ProjectManager();

  // Create a new project, setting up the directory structure and .beproj file
  bool CreateProject(const std::string &directory,
                     const std::string &projectName);

  // Load an existing .beproj file
  bool LoadProject(const std::string &bprojPath);

  // Save the currently open project properties
  bool SaveCurrentProject();

  // Check if a project is currently active
  bool HasActiveProject() const { return m_activeProject.path != ""; }

  const Project &GetActiveProject() const { return m_activeProject; }

  // Unloads the current project
  void CloseProject();

private:
  Project m_activeProject;
};

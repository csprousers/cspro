#pragma once

class CapiQuestionManager;


std::string WriteToYaml(const CapiQuestionManager& question_manager);

void ReadFromYaml(CapiQuestionManager& question_manager, std::istream& input);
void ReadFromYaml(CapiQuestionManager& question_manager, const std::string& input);

##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=spacely
ConfigurationName      :=Debug
WorkspaceConfiguration :=Debug
WorkspacePath          :=/home/alecca/Documents/spacely
ProjectPath            :=/home/alecca/Documents/spacely
IntermediateDirectory  :=$(ConfigurationName)
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=Alexander Forselius
Date                   :=09/04/26
CodeLitePath           :=/home/alecca/.codelite
LinkerName             :=/usr/bin/g++
SharedObjectLinkerName :=/usr/bin/g++ -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputDirectory        :=$(IntermediateDirectory)
OutputFile             :=$(IntermediateDirectory)/$(ProjectName)
Preprocessors          :=
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="spacely.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  $(shell wx-config --libs core,base,aui,xml,xrc)
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). 
IncludePCH             := 
RcIncludePath          := 
Libs                   := 
ArLibs                 :=  
LibPath                := $(LibraryPathSwitch). 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overridden using an environment variable
##
AR       := /usr/bin/ar rcu
CXX      := /usr/bin/g++
CC       := /usr/bin/gcc
CXXFLAGS :=  -gdwarf-2 -O0 -Wall $(shell wx-config --cflags)  $(Preprocessors)
CFLAGS   :=  -gdwarf-2 -O0 -Wall $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/src_SmilTypes.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_ScenePanel.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_SmilView.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_SmilDoc.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_DrawView.cpp$(ObjectSuffix) $(IntermediateDirectory)/wxcrafter.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_KeyframePanel.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_Palette.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_View.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_SpaSaveAsDialog.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/src_DrawCommands.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_SnapEngine.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_HierarchyPanel.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_NewDrawingDialog.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_SpaDoc.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_SnapSettings.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_SpaView.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_SettingsDialog.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_PropPanel.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_MainFrame.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/src_App.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_AssetManagerPanel.cpp$(ObjectSuffix) $(IntermediateDirectory)/wxcrafter_bitmaps.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_DrawDoc.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_Document.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_ColorSwatchPanel.cpp$(ObjectSuffix) 



Objects=$(Objects0) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d $(Objects) 
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	$(LinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)

MakeIntermediateDirs:
	@test -d $(ConfigurationName) || $(MakeDirCommand) $(ConfigurationName)


$(IntermediateDirectory)/.d:
	@test -d $(ConfigurationName) || $(MakeDirCommand) $(ConfigurationName)

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/src_SmilTypes.cpp$(ObjectSuffix): src/SmilTypes.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_SmilTypes.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_SmilTypes.cpp$(DependSuffix) -MM src/SmilTypes.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/src/SmilTypes.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_SmilTypes.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_SmilTypes.cpp$(PreprocessSuffix): src/SmilTypes.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_SmilTypes.cpp$(PreprocessSuffix) src/SmilTypes.cpp

$(IntermediateDirectory)/src_ScenePanel.cpp$(ObjectSuffix): src/ScenePanel.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_ScenePanel.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_ScenePanel.cpp$(DependSuffix) -MM src/ScenePanel.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/src/ScenePanel.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_ScenePanel.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_ScenePanel.cpp$(PreprocessSuffix): src/ScenePanel.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_ScenePanel.cpp$(PreprocessSuffix) src/ScenePanel.cpp

$(IntermediateDirectory)/src_SmilView.cpp$(ObjectSuffix): src/SmilView.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_SmilView.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_SmilView.cpp$(DependSuffix) -MM src/SmilView.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/src/SmilView.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_SmilView.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_SmilView.cpp$(PreprocessSuffix): src/SmilView.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_SmilView.cpp$(PreprocessSuffix) src/SmilView.cpp

$(IntermediateDirectory)/src_SmilDoc.cpp$(ObjectSuffix): src/SmilDoc.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_SmilDoc.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_SmilDoc.cpp$(DependSuffix) -MM src/SmilDoc.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/src/SmilDoc.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_SmilDoc.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_SmilDoc.cpp$(PreprocessSuffix): src/SmilDoc.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_SmilDoc.cpp$(PreprocessSuffix) src/SmilDoc.cpp

$(IntermediateDirectory)/src_DrawView.cpp$(ObjectSuffix): src/DrawView.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_DrawView.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_DrawView.cpp$(DependSuffix) -MM src/DrawView.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/src/DrawView.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_DrawView.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_DrawView.cpp$(PreprocessSuffix): src/DrawView.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_DrawView.cpp$(PreprocessSuffix) src/DrawView.cpp

$(IntermediateDirectory)/wxcrafter.cpp$(ObjectSuffix): wxcrafter.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/wxcrafter.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/wxcrafter.cpp$(DependSuffix) -MM wxcrafter.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/wxcrafter.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wxcrafter.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wxcrafter.cpp$(PreprocessSuffix): wxcrafter.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wxcrafter.cpp$(PreprocessSuffix) wxcrafter.cpp

$(IntermediateDirectory)/src_KeyframePanel.cpp$(ObjectSuffix): src/KeyframePanel.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_KeyframePanel.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_KeyframePanel.cpp$(DependSuffix) -MM src/KeyframePanel.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/src/KeyframePanel.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_KeyframePanel.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_KeyframePanel.cpp$(PreprocessSuffix): src/KeyframePanel.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_KeyframePanel.cpp$(PreprocessSuffix) src/KeyframePanel.cpp

$(IntermediateDirectory)/src_Palette.cpp$(ObjectSuffix): src/Palette.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_Palette.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_Palette.cpp$(DependSuffix) -MM src/Palette.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/src/Palette.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_Palette.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_Palette.cpp$(PreprocessSuffix): src/Palette.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_Palette.cpp$(PreprocessSuffix) src/Palette.cpp

$(IntermediateDirectory)/src_View.cpp$(ObjectSuffix): src/View.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_View.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_View.cpp$(DependSuffix) -MM src/View.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/src/View.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_View.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_View.cpp$(PreprocessSuffix): src/View.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_View.cpp$(PreprocessSuffix) src/View.cpp

$(IntermediateDirectory)/src_SpaSaveAsDialog.cpp$(ObjectSuffix): src/SpaSaveAsDialog.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_SpaSaveAsDialog.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_SpaSaveAsDialog.cpp$(DependSuffix) -MM src/SpaSaveAsDialog.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/src/SpaSaveAsDialog.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_SpaSaveAsDialog.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_SpaSaveAsDialog.cpp$(PreprocessSuffix): src/SpaSaveAsDialog.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_SpaSaveAsDialog.cpp$(PreprocessSuffix) src/SpaSaveAsDialog.cpp

$(IntermediateDirectory)/src_DrawCommands.cpp$(ObjectSuffix): src/DrawCommands.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_DrawCommands.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_DrawCommands.cpp$(DependSuffix) -MM src/DrawCommands.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/src/DrawCommands.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_DrawCommands.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_DrawCommands.cpp$(PreprocessSuffix): src/DrawCommands.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_DrawCommands.cpp$(PreprocessSuffix) src/DrawCommands.cpp

$(IntermediateDirectory)/src_SnapEngine.cpp$(ObjectSuffix): src/SnapEngine.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_SnapEngine.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_SnapEngine.cpp$(DependSuffix) -MM src/SnapEngine.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/src/SnapEngine.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_SnapEngine.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_SnapEngine.cpp$(PreprocessSuffix): src/SnapEngine.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_SnapEngine.cpp$(PreprocessSuffix) src/SnapEngine.cpp

$(IntermediateDirectory)/src_HierarchyPanel.cpp$(ObjectSuffix): src/HierarchyPanel.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_HierarchyPanel.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_HierarchyPanel.cpp$(DependSuffix) -MM src/HierarchyPanel.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/src/HierarchyPanel.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_HierarchyPanel.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_HierarchyPanel.cpp$(PreprocessSuffix): src/HierarchyPanel.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_HierarchyPanel.cpp$(PreprocessSuffix) src/HierarchyPanel.cpp

$(IntermediateDirectory)/src_NewDrawingDialog.cpp$(ObjectSuffix): src/NewDrawingDialog.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_NewDrawingDialog.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_NewDrawingDialog.cpp$(DependSuffix) -MM src/NewDrawingDialog.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/src/NewDrawingDialog.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_NewDrawingDialog.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_NewDrawingDialog.cpp$(PreprocessSuffix): src/NewDrawingDialog.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_NewDrawingDialog.cpp$(PreprocessSuffix) src/NewDrawingDialog.cpp

$(IntermediateDirectory)/src_SpaDoc.cpp$(ObjectSuffix): src/SpaDoc.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_SpaDoc.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_SpaDoc.cpp$(DependSuffix) -MM src/SpaDoc.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/src/SpaDoc.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_SpaDoc.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_SpaDoc.cpp$(PreprocessSuffix): src/SpaDoc.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_SpaDoc.cpp$(PreprocessSuffix) src/SpaDoc.cpp

$(IntermediateDirectory)/src_SnapSettings.cpp$(ObjectSuffix): src/SnapSettings.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_SnapSettings.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_SnapSettings.cpp$(DependSuffix) -MM src/SnapSettings.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/src/SnapSettings.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_SnapSettings.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_SnapSettings.cpp$(PreprocessSuffix): src/SnapSettings.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_SnapSettings.cpp$(PreprocessSuffix) src/SnapSettings.cpp

$(IntermediateDirectory)/src_SpaView.cpp$(ObjectSuffix): src/SpaView.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_SpaView.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_SpaView.cpp$(DependSuffix) -MM src/SpaView.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/src/SpaView.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_SpaView.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_SpaView.cpp$(PreprocessSuffix): src/SpaView.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_SpaView.cpp$(PreprocessSuffix) src/SpaView.cpp

$(IntermediateDirectory)/src_SettingsDialog.cpp$(ObjectSuffix): src/SettingsDialog.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_SettingsDialog.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_SettingsDialog.cpp$(DependSuffix) -MM src/SettingsDialog.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/src/SettingsDialog.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_SettingsDialog.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_SettingsDialog.cpp$(PreprocessSuffix): src/SettingsDialog.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_SettingsDialog.cpp$(PreprocessSuffix) src/SettingsDialog.cpp

$(IntermediateDirectory)/src_PropPanel.cpp$(ObjectSuffix): src/PropPanel.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_PropPanel.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_PropPanel.cpp$(DependSuffix) -MM src/PropPanel.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/src/PropPanel.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_PropPanel.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_PropPanel.cpp$(PreprocessSuffix): src/PropPanel.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_PropPanel.cpp$(PreprocessSuffix) src/PropPanel.cpp

$(IntermediateDirectory)/src_MainFrame.cpp$(ObjectSuffix): src/MainFrame.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_MainFrame.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_MainFrame.cpp$(DependSuffix) -MM src/MainFrame.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/src/MainFrame.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_MainFrame.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_MainFrame.cpp$(PreprocessSuffix): src/MainFrame.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_MainFrame.cpp$(PreprocessSuffix) src/MainFrame.cpp

$(IntermediateDirectory)/src_App.cpp$(ObjectSuffix): src/App.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_App.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_App.cpp$(DependSuffix) -MM src/App.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/src/App.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_App.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_App.cpp$(PreprocessSuffix): src/App.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_App.cpp$(PreprocessSuffix) src/App.cpp

$(IntermediateDirectory)/src_AssetManagerPanel.cpp$(ObjectSuffix): src/AssetManagerPanel.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_AssetManagerPanel.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_AssetManagerPanel.cpp$(DependSuffix) -MM src/AssetManagerPanel.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/src/AssetManagerPanel.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_AssetManagerPanel.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_AssetManagerPanel.cpp$(PreprocessSuffix): src/AssetManagerPanel.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_AssetManagerPanel.cpp$(PreprocessSuffix) src/AssetManagerPanel.cpp

$(IntermediateDirectory)/wxcrafter_bitmaps.cpp$(ObjectSuffix): wxcrafter_bitmaps.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/wxcrafter_bitmaps.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/wxcrafter_bitmaps.cpp$(DependSuffix) -MM wxcrafter_bitmaps.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/wxcrafter_bitmaps.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wxcrafter_bitmaps.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wxcrafter_bitmaps.cpp$(PreprocessSuffix): wxcrafter_bitmaps.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wxcrafter_bitmaps.cpp$(PreprocessSuffix) wxcrafter_bitmaps.cpp

$(IntermediateDirectory)/src_DrawDoc.cpp$(ObjectSuffix): src/DrawDoc.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_DrawDoc.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_DrawDoc.cpp$(DependSuffix) -MM src/DrawDoc.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/src/DrawDoc.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_DrawDoc.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_DrawDoc.cpp$(PreprocessSuffix): src/DrawDoc.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_DrawDoc.cpp$(PreprocessSuffix) src/DrawDoc.cpp

$(IntermediateDirectory)/src_Document.cpp$(ObjectSuffix): src/Document.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_Document.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_Document.cpp$(DependSuffix) -MM src/Document.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/src/Document.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_Document.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_Document.cpp$(PreprocessSuffix): src/Document.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_Document.cpp$(PreprocessSuffix) src/Document.cpp

$(IntermediateDirectory)/src_ColorSwatchPanel.cpp$(ObjectSuffix): src/ColorSwatchPanel.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_ColorSwatchPanel.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_ColorSwatchPanel.cpp$(DependSuffix) -MM src/ColorSwatchPanel.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/alecca/Documents/spacely/src/ColorSwatchPanel.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_ColorSwatchPanel.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_ColorSwatchPanel.cpp$(PreprocessSuffix): src/ColorSwatchPanel.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_ColorSwatchPanel.cpp$(PreprocessSuffix) src/ColorSwatchPanel.cpp


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r $(ConfigurationName)/



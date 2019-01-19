
#include <glm/glm.hpp>

#include "ImGuiUI.h"

#include "CmdBufHandler.h"
#include "Face.h"
#include "Helpers.h"
#include "SceneHandler.h"

void ImGuiUI::draw(std::unique_ptr<RenderPass::Base>& pPass, uint8_t frameIndex) {
    // if (pPass->data.tests[frameIndex] == 0) {
    //    return;
    //}

    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    appMainMenuBar();
    if (showDemoWindow_) ImGui::ShowDemoWindow(&showDemoWindow_);
    if (showSelectionInfoWindow_) showSelectionInfoWindow(&showSelectionInfoWindow_);

    // Rendering
    ImGui::Render();

    auto& cmd = pPass->data.priCmds[frameIndex];
    pPass->beginPass(frameIndex, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

    // Record Imgui Draw Data and draw funcs into command buffer
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd, frameIndex);

    pPass->endPass(frameIndex);
}

void ImGuiUI::reset() {
    // vkFreeCommandBuffers(ctx_.dev, CmdBufHandler::present_cmd_pool(), 1, &inst_.cmd_);
}

void ImGuiUI::appMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            menuFile();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "CTRL+Z")) {
            }
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {
            }  // Disabled item
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X")) {
            }
            if (ImGui::MenuItem("Copy", "CTRL+C")) {
            }
            if (ImGui::MenuItem("Paste", "CTRL+V")) {
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Show Windows")) {
            menuShowWindows();
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void ImGuiUI::menuFile() {
    // ImGui::MenuItem("(dummy menu)", NULL, false, false);
    if (ImGui::MenuItem("New")) {
    }
    if (ImGui::MenuItem("Open", "Ctrl+O")) {
    }
    if (ImGui::BeginMenu("Open Recent")) {
        // ImGui::MenuItem("fish_hat.c");
        // ImGui::MenuItem("fish_hat.inl");
        // ImGui::MenuItem("fish_hat.h");
        // if (ImGui::BeginMenu("More..")) {
        //    ImGui::MenuItem("Hello");
        //    ImGui::MenuItem("Sailor");
        //    if (ImGui::BeginMenu("Recurse..")) {
        //        ShowExampleMenuFile();
        //        ImGui::EndMenu();
        //    }
        //    ImGui::EndMenu();
        //}
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Save", "Ctrl+S")) {
    }
    if (ImGui::MenuItem("Save As..")) {
    }
    if (ImGui::MenuItem("Quit", "Alt+F4")) {
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
    }
}

void ImGuiUI::menuShowWindows() {
    if (ImGui::MenuItem("Selection Information", nullptr, &showSelectionInfoWindow_)) {
    }
    if (ImGui::MenuItem("Demo Window", nullptr, &showDemoWindow_)) {
    }
}

void ImGuiUI::showSelectionInfoWindow(bool* p_open) {
    // We specify a default position/size in case there's no data in the .ini file. Typically this isn't required! We only do
    // it to make the Demo applications a little more welcoming.
    ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Selection Information", p_open, 0)) {
        // Early out if the window is collapsed, as an optimization.
        ImGui::End();
        return;
    }

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    ImGui::Separator();

    ImGui::Text("FACES:");
    ImGui::Separator();
    showFaceSelectionInfoText(SceneHandler::getFaceSelectionFace());

    // End of showSelectionInfoWindow()
    ImGui::End();
}

void ImGuiUI::showFaceSelectionInfoText(const std::unique_ptr<Face>& pFace) {
    if (pFace == nullptr) {
        ImGui::Text("No face...");
        return;
    }

    // MISCELLANEOUS
    ImGui::Text("Miscellaneous:");
    ImGui::Columns(2, "vertex_misc_columns", false);  // 2-ways, no border
    ImGui::SetColumnWidth(0, 200.0f), ImGui::Indent();

    auto calcNormal = helpers::triangleNormal((*pFace)[0].position, (*pFace)[1].position, (*pFace)[2].position);
    ImGui::Text("Calculated normal:"), ImGui::NextColumn();
    ImGui::Text("(%.5f, %.5f, %.5f)", calcNormal.x, calcNormal.y, calcNormal.z), ImGui::NextColumn();

    ImGui::Unindent(), ImGui::Columns(1);

    // VERTEX
    ImGui::Text("Vertex:"), ImGui::Separator();
    ImGui::Columns(2, "vertex_columns", true);  // 2-ways, no border
    ImGui::SetColumnWidth(0, 200.0f), ImGui::Indent();
    for (uint8_t i = 0; i < 3; i++) {
        const auto& v = (*pFace)[i];

        ImGui::Text("Index:"), ImGui::NextColumn();
        ImGui::Text("%d", pFace->getIndex(i)), ImGui::NextColumn();

        ImGui::Text("Position:"), ImGui::NextColumn();
        ImGui::Text("(%.5f, %.5f, %.5f)", v.position.x, v.position.y, v.position.z), ImGui::NextColumn();

        ImGui::Text("Normal:"), ImGui::NextColumn();
        ImGui::Text("(%.5f, %.5f, %.5f)", v.normal.x, v.normal.y, v.normal.z), ImGui::NextColumn();

        ImGui::Text("Color:"), ImGui::NextColumn();
        ImGui::Text("(%.5f, % .5f, %.5f, %.5f)", v.color.x, v.color.y, v.color.z, v.color.w), ImGui::NextColumn();

        ImGui::Text("Tex coord:"), ImGui::NextColumn();
        ImGui::Text("(%.5f, %.5f)", v.texCoord.x, v.texCoord.y), ImGui::NextColumn();

        ImGui::Text("Tangent:"), ImGui::NextColumn();
        ImGui::Text("(%.5f, %.5f, %.5f)", v.tangent.x, v.tangent.y, v.tangent.z), ImGui::NextColumn();

        ImGui::Text("Binormal:"), ImGui::NextColumn();
        ImGui::Text("(%.5f, %.5f, %.5f)", v.binormal.x, v.binormal.y, v.binormal.z), ImGui::NextColumn();

        ImGui::Text("Smoothing group:"), ImGui::NextColumn();
        ImGui::Text("%d", v.smoothingGroupId), ImGui::NextColumn();

        ImGui::Separator();
    }
    ImGui::Unindent(), ImGui::Columns(1);
}

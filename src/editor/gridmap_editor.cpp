#include "core/allocator.h"
#include "editor/action.h"
#include "editor/asset_browser.h"
#include "editor/studio_app.h"
#include "editor/world_editor.h"
#include "engine/component_uid.h"
#include "engine/engine.h"
#include "engine/resource_manager.h"
#include "renderer/model.h"
#include "renderer/render_module.h"

#include "imgui/imgui.h"

namespace Lumix {

static const ComponentType MODEL_INSTANCE_TYPE = reflection::getComponentType("model_instance");

struct EditorPlugin : StudioApp::GUIPlugin, StudioApp::MousePlugin {
	EditorPlugin(StudioApp& app) 
		: m_app(app)
		, m_allocator(app.getAllocator(), "gridmap")
		, m_models(m_allocator)
	{}

	bool getIntersectPlanePos(DVec3& pos) {
		WorldEditor& editor = m_app.getWorldEditor();
		WorldView& view = editor.getView();
		const Vec2 mp = view.getMousePos();
		const Ray ray = view.getViewport().getRay(mp);
		const float y = m_floor * m_cell_size.y + m_grid_offset;
		float t;
		if (!getRayPlaneIntersecion(Vec3(ray.origin), ray.dir, Vec3(0, y, 0), Vec3(0, 1, 0), t)) return false;

		pos = ray.origin + ray.dir * t;

		// align to cell
		pos.x = m_cell_size.x == 0 ? pos.x : floor(pos.x / m_cell_size.x) * m_cell_size.x;
		pos.y = m_floor * m_cell_size.y;
		pos.z = m_cell_size.z == 0 ? pos.z : floor(pos.z / m_cell_size.z) * m_cell_size.z;

		pos.x += m_cell_size.x * 0.5f;
		pos.z += m_cell_size.z * 0.5f;
		
		return true;
	}

	void onGUI() override {
		if (ImGui::Begin("Gridmap")) {
			ImGuiEx::Label("Enabled");
			ImGui::Checkbox("##enabled", &m_enabled);
			if (m_enabled) {
				ImGuiEx::Label("Cell size");
				ImGui::InputFloat3("##cell_size", &m_cell_size.x);
				ImGuiEx::Label("Grid offset");
				ImGui::InputFloat("##grid_offset", &m_grid_offset);
				ImGuiEx::Label("Floor");
				ImGui::InputInt("##floor", (i32*)&m_floor);
				
				ImGui::Separator();
				AssetBrowser& asset_browser = m_app.getAssetBrowser();
				Path path;
				ImGui::TextUnformatted(ICON_FA_PLUS); ImGui::SameLine();
				if (asset_browser.resourceInput("new resource", path, Model::TYPE)) {
					Engine& engine = m_app.getEngine();
					Model* m = engine.getResourceManager().load<Model>(path);
					m_models.push(m);
				}
				CommonActions& common = m_app.getCommonActions();
				if (m_app.checkShortcut(common.del) && m_selected < (u32)m_models.size()) {
					m_models[m_selected]->decRefCount();
					m_models.erase(m_selected);
				}
				const u32 num_cols = u32(ImGui::GetContentRegionAvail().x / (asset_browser.getThumbnailWidth() + ImGui::GetStyle().ItemSpacing.x));
				for (Model*& m : m_models) {
					const u32 idx = u32(&m - m_models.begin());
					if (idx % num_cols != 0) ImGui::SameLine();
					asset_browser.tile(m->getPath(), m_selected == idx);
					if (ImGui::IsItemClicked()) {
						if (m_selected == idx) m_selected = 0xffFFffFF;
						else m_selected = idx;
					}
				}

				WorldEditor& editor = m_app.getWorldEditor();

				DVec3 pos;
				if (getIntersectPlanePos(pos)) {
					RenderModule* module = (RenderModule*)editor.getWorld()->getModule("renderer");
					const float half = 0.5f * 5;
					const u32 color = 0xff00ffff;
					for (i32 i = 0; i <= 5; ++i) {
						float z = (i - half) * m_cell_size.z;
						DVec3 from = pos + Vec3(-half * m_cell_size.x, m_grid_offset, z);
						DVec3 to = pos + Vec3(half * m_cell_size.x, m_grid_offset, z);
						module->addDebugLine(from, to, color);
						
						float x = (i - half) * m_cell_size.x;
						from = pos + Vec3(x, m_grid_offset, -half * m_cell_size.z);
						to = pos + Vec3(x, m_grid_offset, half * m_cell_size.z);
						module->addDebugLine(from, to, color);
					}
				}
			}
		}
		ImGui::End();
	}
	
	const char* getName() const override { return "gridmap"; }

	bool onMouseDown(WorldView& view, int x, int y) override { 
		if (!m_enabled) return false;
		if (m_selected >= (u32)m_models.size()) return false;

		WorldEditor& editor = m_app.getWorldEditor();
		
		DVec3 pos;
		if (getIntersectPlanePos(pos)) {
			editor.beginCommandGroup("gridmap_add");
			EntityRef e = editor.addEntity();
			editor.addComponent(Span(&e, 1), MODEL_INSTANCE_TYPE);
			editor.setProperty(MODEL_INSTANCE_TYPE, nullptr, -1, "Source", Span(&e, 1), m_models[m_selected]->getPath());
			editor.setEntitiesPositions(&e, &pos, 1);
			editor.endCommandGroup();
		}
		
		return true;
	}
	void onMouseUp(WorldView& view, int x, int y, os::MouseButton button) override {}
	void onMouseMove(WorldView& view, int x, int y, int rel_x, int rel_y) override {}
	void onMouseWheel(float value) override {}

	StudioApp& m_app;
	TagAllocator m_allocator;
	Array<Model*> m_models;
	u32 m_selected = 0;
	u32 m_floor = 0;
	Vec3 m_cell_size = Vec3(1);
	float m_grid_offset = 0;
	bool m_enabled = true;
};


LUMIX_STUDIO_ENTRY(gridmap) {
	auto* plugin = LUMIX_NEW(app.getAllocator(), EditorPlugin)(app);
	app.addPlugin((StudioApp::GUIPlugin&)*plugin);
	app.addPlugin((StudioApp::MousePlugin&)*plugin);
	return nullptr;
}

}
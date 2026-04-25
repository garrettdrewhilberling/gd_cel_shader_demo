# GD_Cel_Shade_Demo

Developed with Unreal Engine 5

A demo project containing the **GD_CelShading** plugin — a cel-shading post-process material for use with a Post Process Volume.

---

## How to use it

### Copying to another project (e.g. GooseAndDuke)

1. Copy the entire `Plugins/GD_CelShading/` folder into the target project's `Plugins/` folder.
2. Right-click the target project's `.uproject` → **Generate Visual Studio project files**.
3. Open the project in Unreal Editor — it will prompt you to enable the plugin. Accept.
4. In the target project, add a **Post Process Volume** and assign the post-process material:
   - **Post Process Material** → `Plugin/GD_CelShading/M_CelShading4_Inst`

### Fixing asset paths (making the plugin fully portable)

> **Important:** The `.uasset` files internally store their original source path (`/Game/Materials/...` from the GooseAndDuke project). When Unreal Editor opens the project with the plugin, it may warn about redirectors.

To fix this properly:

1. Open **this project** (`GD_Cel_Shade_Demo`) in Unreal Editor.
2. In the **Content Browser**, navigate to the plugin content (`GD_CelShading` folder).
3. Right-click the assets → **Asset Actions → Fix Up Redirectors**.

This rebakes the internal references from `/Game/Materials/...` to `/GD_CelShading/...`, making the plugin fully self-contained and portable.

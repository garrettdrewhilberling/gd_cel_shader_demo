try:
    import unreal

    _folder = "/GD_CelShading"

    _ar = unreal.AssetRegistryHelpers.get_asset_registry()
    _ar.wait_for_completion()

    _filter = unreal.ARFilter(
        class_names=["ObjectRedirector"],
        package_paths=[_folder],
        recursive_paths=True
    )

    _asset_data_list = _ar.get_assets(_filter)

    _redirectors = []
    for _ad in _asset_data_list:
        _obj = unreal.load_asset(f"{_ad.package_name}.{_ad.asset_name}")
        if _obj is not None:
            _redirectors.append(_obj)

    if _redirectors:
        unreal.AssetToolsHelpers.get_asset_tools().fix_up_redirectors(_redirectors)

except Exception:
    pass

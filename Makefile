.PHONY: clean All

All:
	@echo "----------Building project:[ spacely - Debug ]----------"
	@"$(MAKE)" -f  "spacely.mk"
clean:
	@echo "----------Cleaning project:[ spacely - Debug ]----------"
	@"$(MAKE)" -f  "spacely.mk" clean

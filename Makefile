.first: all

ifeq ($(V), 1)
    CMAKE:=cmake --verbose
    RM:=rm
else
    CMAKE:=@cmake
    RM:=@rm
endif

BUILD_FOLDER:=.build-$(shell uname -p)

all: test
	@echo make $@;

$(BUILD_FOLDER):
	@echo make $@;
	$(CMAKE) -S. -B$@ -GNinja || $(RM) -f $@/CMakeCache.txt;

test: | $(BUILD_FOLDER)
	@echo make $@;
	$(CMAKE) --build $(BUILD_FOLDER);

clean:
	@echo make $@;
	$(CMAKE) --clean $(BUILD_FOLDER);

distclean:
	@echo make $@;
	$(RM) -rf $(BUILD_FOLDER) *.log;


.PHONY: .first all tests clean distclean

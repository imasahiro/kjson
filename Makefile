target=Debug
target=Release
.PHONY : all
all:
	make -C $(target) -j8
.PHONY : clean
clean:
	make -C $(target) clean
.PHONY : install
install:
	make -C $(target) install
.PHONY : test
test:
	make -C $(target) test

#makefile�ļ�����ָ��
#���make�ļ�����makefile��ֱ��ʹ��make�Ϳ��Ա���
#���make�ļ�������makefile������test.txt����ôʹ��make -f test.txt

#------------------------------------------������ϵͳ32λ64λ--------------------------------------------------------
#SYS_BIT=$(shell getconf LONG_BIT)
#SYS_BIT=$(shell getconf WORD_BIT)
SYS_BIT=$(shell getconf LONG_BIT)
ifeq ($(SYS_BIT),32)
	CPU =  -march=i686 
else 
	CPU = 
endif

#������������������������������ģ�顪����������������������#
#����һ��(make -C subdir) 
#��������(cd subdir && make)
#��-w ���Բ鿴ִ��ָ��ʱ����ǰ����Ŀ¼��Ϣ

#����������������������������Exist������������������������#
MDK_DIR = ./Micro-Development-Kit

all: $(MDK_DIR)/lib/mdk.a
	(cd Exist && make -w)


#����������������������������mdk������������������������#
$(MDK_DIR)/lib/mdk.a:
	@echo "Complie mdk"
	(cd $(MDK_DIR)/mdk_static && make -w)
	@echo ""
	@echo "mdk Complie finished"
	@echo ""
	@echo ""
	@echo ""
	@echo ""

#����������������������������״̬������������������������������#
stateser:
	@echo "Complie mdk"
	(cd $(MDK_DIR)/mdk_static && make -w)
	@echo ""
	@echo "mdk Complie finished"
	@echo ""
	@echo ""
	@echo ""
	@echo ""

